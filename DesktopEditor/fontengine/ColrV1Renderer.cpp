/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#include "ColrV1Renderer.h"

#include FT_COLOR_H
#include FT_OUTLINE_H
#include <freetype/internal/ftobjs.h> // FT_New_GlyphSlot, FontFile.cpp uses it too

#include <algorithm>
#include <cmath>
#include <new>

namespace
{
	// ponytail: whole glyph rendered in float layers (16 bytes/pixel), refuse
	// above this and let the caller fall back to the outline. Tile if larger
	// zoom levels need color.
	const int c_nMaxPixels = 1024 * 1024;
	const int c_nMaxDepth  = 64;
	// Fonts are untrusted (embedded in documents). A paint graph can fan out
	// (shared children, huge layer lists), so paint nodes and passes over the
	// glyph are capped; the most complex Noto Color Emoji glyphs need about a
	// quarter of the pass budget. Same idea for live temporary layers and
	// gradient stops.
	const long long c_nMaxPixelOps   = 1LL << 30;
	const int       c_nMaxNodes      = 100000;
	const long long c_nMaxLiveBytes  = 160LL * 1024 * 1024;
	const size_t    c_nMaxColorStops = 256;
	const double    c_dMaxCoord      = 1e6; // device pixels, far beyond c_nMaxPixels
	const double c_dPi = 3.14159265358979323846;

	struct Color { float r = 0, g = 0, b = 0, a = 0; }; // premultiplied

	// x' = a*x + c*y + e, y' = b*x + d*y + f
	struct Matrix
	{
		double a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;

		Matrix operator*(const Matrix& m) const // this applied after m
		{
			Matrix r;
			r.a = a * m.a + c * m.b;
			r.b = b * m.a + d * m.b;
			r.c = a * m.c + c * m.d;
			r.d = b * m.c + d * m.d;
			r.e = a * m.e + c * m.f + e;
			r.f = b * m.e + d * m.f + f;
			return r;
		}
		bool Invert(Matrix& r) const
		{
			double det = a * d - b * c;
			if (std::fabs(det) < 1e-12)
				return false;
			r.a = d / det;
			r.b = -b / det;
			r.c = -c / det;
			r.d = a / det;
			r.e = (c * f - d * e) / det;
			r.f = (b * e - a * f) / det;
			return true;
		}
		static Matrix Translate(double x, double y) { Matrix m; m.e = x; m.f = y; return m; }
		static Matrix Around(const Matrix& m, double cx, double cy) { return Translate(cx, cy) * m * Translate(-cx, -cy); }
	};

	inline double Fx(FT_Fixed v) { return v / 65536.0; }
	inline float F2Dot14(FT_F2Dot14 v) { return v / 16384.0f; }

	typedef std::vector<Color> Layer;

	// Transform paints (PaintTransform, Translate, Scale, Rotate, Skew): the
	// matrix they apply and the paint they wrap
	bool ChildTransform(const FT_COLR_Paint& paint, Matrix& m, FT_OpaquePaint& child)
	{
		switch (paint.format)
		{
		case FT_COLR_PAINTFORMAT_TRANSFORM:
		{
			const FT_Affine23& t = paint.u.transform.affine;
			m.a = Fx(t.xx); m.c = Fx(t.xy); m.e = Fx(t.dx);
			m.b = Fx(t.yx); m.d = Fx(t.yy); m.f = Fx(t.dy);
			child = paint.u.transform.paint;
			return true;
		}
		case FT_COLR_PAINTFORMAT_TRANSLATE:
			m = Matrix::Translate(Fx(paint.u.translate.dx), Fx(paint.u.translate.dy));
			child = paint.u.translate.paint;
			return true;
		case FT_COLR_PAINTFORMAT_SCALE:
		{
			const FT_PaintScale& sc = paint.u.scale;
			Matrix k;
			k.a = Fx(sc.scale_x);
			k.d = Fx(sc.scale_y);
			m = Matrix::Around(k, Fx(sc.center_x), Fx(sc.center_y));
			child = sc.paint;
			return true;
		}
		case FT_COLR_PAINTFORMAT_ROTATE:
		{
			const FT_PaintRotate& r = paint.u.rotate;
			double rad = Fx(r.angle) * c_dPi;
			Matrix k;
			k.a = std::cos(rad); k.c = -std::sin(rad);
			k.b = std::sin(rad); k.d = std::cos(rad);
			m = Matrix::Around(k, Fx(r.center_x), Fx(r.center_y));
			child = r.paint;
			return true;
		}
		case FT_COLR_PAINTFORMAT_SKEW:
		{
			const FT_PaintSkew& sk = paint.u.skew;
			Matrix k;
			k.c = -std::tan(Fx(sk.x_skew_angle) * c_dPi);
			k.b = std::tan(Fx(sk.y_skew_angle) * c_dPi);
			m = Matrix::Around(k, Fx(sk.center_x), Fx(sk.center_y));
			child = sk.paint;
			return true;
		}
		default:
			return false;
		}
	}

	struct Stop { float offset; Color color; };

	// 26.6, clamped: NaN or huge values from accumulated paint transforms
	FT_Pos ToPos(double v)
	{
		if (!(v > -c_dMaxCoord))
			v = -c_dMaxCoord;
		else if (!(v < c_dMaxCoord))
			v = c_dMaxCoord;
		return (FT_Pos)std::lround(v * 64);
	}

	// font units -> device pixels (y-up), including FT_Set_Transform
	Matrix DeviceMatrix(FT_Face face)
	{
		FT_Matrix ftm;
		FT_Vector delta;
		FT_Get_Transform(face, &ftm, &delta);
		Matrix scale;
		scale.a = face->size->metrics.x_scale / 65536.0 / 64.0;
		scale.d = face->size->metrics.y_scale / 65536.0 / 64.0;
		Matrix transform;
		transform.a = Fx(ftm.xx); transform.c = Fx(ftm.xy);
		transform.b = Fx(ftm.yx); transform.d = Fx(ftm.yy);
		transform.e = delta.x / 64.0; transform.f = delta.y / 64.0;
		return transform * scale;
	}

	class CRenderer
	{
	public:
		FT_Face  m_pFace;
		int      m_nW, m_nH;
		FT_Color* m_pPalette = nullptr;
		Color    m_oForeground;
		int      m_nDepth = 0;
		const void* m_arStack[c_nMaxDepth] = {};
		long long m_nBudget    = c_nMaxPixelOps;
		int      m_nNodes      = 0;
		long long m_nLiveBytes = 0;
		bool     m_bAborted    = false;

		// reserves memory for temporary buffers, false (and abort) above the cap
		bool Reserve(long long bytes)
		{
			if (m_nLiveBytes + bytes > c_nMaxLiveBytes)
			{
				m_bAborted = true;
				return false;
			}
			m_nLiveBytes += bytes;
			return true;
		}
		void Release(long long bytes) { m_nLiveBytes -= bytes; }

		CRenderer(FT_Face face, int w, int h, unsigned int fg) : m_pFace(face), m_nW(w), m_nH(h)
		{
			float a = (fg & 0xFF) / 255.0f;
			m_oForeground = { ((fg >> 24) & 0xFF) / 255.0f * a, ((fg >> 16) & 0xFF) / 255.0f * a, ((fg >> 8) & 0xFF) / 255.0f * a, a };
			if (FT_Palette_Select(face, 0, &m_pPalette))
				m_pPalette = nullptr;
		}

		Color Resolve(const FT_ColorIndex& idx) const
		{
			float alpha = std::min(1.0f, std::max(0.0f, F2Dot14(idx.alpha)));
			Color c;
			if (idx.palette_index == 0xFFFF)
				c = m_oForeground;
			else if (m_pPalette)
			{
				FT_Palette_Data data;
				if (!FT_Palette_Data_Get(m_pFace, &data) && idx.palette_index < data.num_palette_entries)
				{
					const FT_Color& p = m_pPalette[idx.palette_index];
					float a = p.alpha / 255.0f;
					c = { p.red / 255.0f * a, p.green / 255.0f * a, p.blue / 255.0f * a, a };
				}
			}
			c.r *= alpha; c.g *= alpha; c.b *= alpha; c.a *= alpha;
			return c;
		}

		// Coverage mask of an outline glyph, in [0..1], same size as layers
		bool GlyphMask(FT_UInt gid, const Matrix& ctm, std::vector<float>& mask)
		{
			if (FT_Load_Glyph(m_pFace, gid, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_TRANSFORM))
				return false;
			FT_GlyphSlot slot = m_pFace->glyph;
			if (slot->format != FT_GLYPH_FORMAT_OUTLINE)
				return false;

			FT_Outline outline;
			if (FT_Outline_New(slot->library, slot->outline.n_points, slot->outline.n_contours, &outline))
				return false;
			FT_Outline_Copy(&slot->outline, &outline);
			for (int i = 0; i < outline.n_points; ++i)
			{
				double x = (double)outline.points[i].x, y = (double)outline.points[i].y;
				outline.points[i].x = ToPos(ctm.a * x + ctm.c * y + ctm.e);
				outline.points[i].y = ToPos(ctm.b * x + ctm.d * y + ctm.f);
			}

			std::vector<unsigned char> gray((size_t)m_nW * m_nH, 0);
			FT_Bitmap bitmap = {};
			bitmap.rows       = (unsigned int)m_nH;
			bitmap.width      = (unsigned int)m_nW;
			bitmap.pitch      = m_nW;
			bitmap.buffer     = gray.data();
			bitmap.num_grays  = 256;
			bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
			FT_Error err = FT_Outline_Get_Bitmap(slot->library, &outline, &bitmap);
			FT_Outline_Done(slot->library, &outline);
			if (err)
				return false;

			mask.resize(gray.size());
			for (size_t i = 0; i < gray.size(); ++i)
				mask[i] = gray[i] / 255.0f;
			return true;
		}

		void ReadStops(FT_ColorLine& line, std::vector<Stop>& stops)
		{
			FT_ColorStop stop;
			while (stops.size() < c_nMaxColorStops && FT_Get_Colorline_Stops(m_pFace, &stop, &line.color_stop_iterator))
				stops.push_back({ (float)Fx(stop.stop_offset), Resolve(stop.color) });
			std::stable_sort(stops.begin(), stops.end(), [](const Stop& l, const Stop& r) { return l.offset < r.offset; });
		}

		static Color Sample(const std::vector<Stop>& stops, FT_PaintExtend extend, float t)
		{
			float s0 = stops.front().offset, s1 = stops.back().offset;
			if (s1 - s0 < 1e-6f)
				return t < s0 ? stops.front().color : stops.back().color;

			float u = (t - s0) / (s1 - s0);
			switch (extend)
			{
			case FT_COLR_PAINT_EXTEND_REPEAT:
				u = u - std::floor(u);
				break;
			case FT_COLR_PAINT_EXTEND_REFLECT:
			{
				float m = std::fmod(std::fabs(u), 2.0f);
				u = m > 1.0f ? 2.0f - m : m;
				break;
			}
			default:
				u = std::min(1.0f, std::max(0.0f, u));
			}
			float o = s0 + u * (s1 - s0);

			if (o <= stops.front().offset)
				return stops.front().color;
			for (size_t i = 1; i < stops.size(); ++i)
			{
				if (o <= stops[i].offset)
				{
					const Stop& p = stops[i - 1];
					const Stop& n = stops[i];
					float k = (n.offset - p.offset) < 1e-6f ? 1.0f : (o - p.offset) / (n.offset - p.offset);
					return { p.color.r + (n.color.r - p.color.r) * k, p.color.g + (n.color.g - p.color.g) * k,
							 p.color.b + (n.color.b - p.color.b) * k, p.color.a + (n.color.a - p.color.a) * k };
				}
			}
			return stops.back().color;
		}

		static void Over(Color& dst, const Color& src, float cov)
		{
			float sa = src.a * cov;
			float k = 1.0f - sa;
			dst.r = src.r * cov + dst.r * k;
			dst.g = src.g * cov + dst.g * k;
			dst.b = src.b * cov + dst.b * k;
			dst.a = sa + dst.a * k;
		}

		template <typename F>
		void FillEach(Layer& dst, const float* clip, const Matrix& ctm, F colorAt)
		{
			Matrix inv;
			if (!ctm.Invert(inv))
				return;
			for (int y = 0; y < m_nH; ++y)
			{
				// layer row 0 is the top; paint space is y-up with origin at the bottom
				double py = m_nH - y - 0.5;
				for (int x = 0; x < m_nW; ++x)
				{
					size_t i = (size_t)y * m_nW + x;
					float cov = clip ? clip[i] : 1.0f;
					if (cov <= 0.0f)
						continue;
					double px = x + 0.5;
					double gx = inv.a * px + inv.c * py + inv.e;
					double gy = inv.b * px + inv.d * py + inv.f;
					Over(dst[i], colorAt(gx, gy), cov);
				}
			}
		}

		void Draw(const FT_OpaquePaint& opaque, const Matrix& ctm, const float* clip, Layer& dst)
		{
			if (m_bAborted)
				return;
			if (m_nDepth >= c_nMaxDepth || ++m_nNodes > c_nMaxNodes || std::find(m_arStack, m_arStack + m_nDepth, opaque.p) != m_arStack + m_nDepth)
			{
				// too deep, too much work or a cycle: the caller falls back to the outline
				m_bAborted = true;
				return;
			}
			FT_COLR_Paint paint;
			if (!FT_Get_Paint(m_pFace, opaque, &paint))
				return;
			m_arStack[m_nDepth++] = opaque.p;
			DrawPaint(paint, ctm, clip, dst);
			--m_nDepth;
		}

		// one pass over the glyph, false (and abort) when the budget is spent
		bool Charge()
		{
			m_nBudget -= (long long)m_nW * m_nH;
			if (m_nBudget < 0)
				m_bAborted = true;
			return !m_bAborted;
		}

		void DrawPaint(FT_COLR_Paint& paint, const Matrix& ctm, const float* clip, Layer& dst)
		{
			switch (paint.format)
			{
			case FT_COLR_PAINTFORMAT_SOLID:
			case FT_COLR_PAINTFORMAT_LINEAR_GRADIENT:
			case FT_COLR_PAINTFORMAT_RADIAL_GRADIENT:
			case FT_COLR_PAINTFORMAT_SWEEP_GRADIENT:
			case FT_COLR_PAINTFORMAT_GLYPH:
			case FT_COLR_PAINTFORMAT_COMPOSITE:
				if (!Charge())
					return;
				break;
			default:
				break;
			}

			switch (paint.format)
			{
			case FT_COLR_PAINTFORMAT_COLR_LAYERS:
			{
				FT_OpaquePaint layer;
				layer.p = nullptr;
				FT_LayerIterator it = paint.u.colr_layers.layer_iterator;
				while (FT_Get_Paint_Layers(m_pFace, &it, &layer))
					Draw(layer, ctm, clip, dst);
				break;
			}
			case FT_COLR_PAINTFORMAT_SOLID:
			{
				Color c = Resolve(paint.u.solid.color);
				for (size_t i = 0; i < dst.size(); ++i)
				{
					float cov = clip ? clip[i] : 1.0f;
					if (cov > 0.0f)
						Over(dst[i], c, cov);
				}
				break;
			}
			case FT_COLR_PAINTFORMAT_LINEAR_GRADIENT:
			{
				FT_PaintLinearGradient& g = paint.u.linear_gradient;
				std::vector<Stop> stops;
				ReadStops(g.colorline, stops);
				if (stops.empty())
					break;
				double x0 = Fx(g.p0.x), y0 = Fx(g.p0.y);
				double x1 = Fx(g.p1.x), y1 = Fx(g.p1.y);
				double x2 = Fx(g.p2.x), y2 = Fx(g.p2.y);
				// p3: p1 projected onto the line through p0 perpendicular to p0p2
				double nx = -(y2 - y0), ny = x2 - x0;
				double nn = nx * nx + ny * ny;
				double x3 = x1, y3 = y1;
				if (nn > 1e-12)
				{
					double k = ((x1 - x0) * nx + (y1 - y0) * ny) / nn;
					x3 = x0 + k * nx;
					y3 = y0 + k * ny;
				}
				double dx = x3 - x0, dy = y3 - y0, dd = dx * dx + dy * dy;
				FT_PaintExtend ext = g.colorline.extend;
				FillEach(dst, clip, ctm, [&](double x, double y) {
					float t = dd > 1e-12 ? (float)(((x - x0) * dx + (y - y0) * dy) / dd) : 0.0f;
					return Sample(stops, ext, t);
				});
				break;
			}
			case FT_COLR_PAINTFORMAT_RADIAL_GRADIENT:
			{
				FT_PaintRadialGradient& g = paint.u.radial_gradient;
				std::vector<Stop> stops;
				ReadStops(g.colorline, stops);
				if (stops.empty())
					break;
				double cx0 = Fx(g.c0.x), cy0 = Fx(g.c0.y), r0 = Fx(g.r0);
				double cdx = Fx(g.c1.x) - cx0, cdy = Fx(g.c1.y) - cy0, dr = Fx(g.r1) - r0;
				double A = cdx * cdx + cdy * cdy - dr * dr;
				FT_PaintExtend ext = g.colorline.extend;
				FillEach(dst, clip, ctm, [&](double x, double y) {
					// largest t with |p - c(t)| = r(t) and r(t) >= 0 (two-point conical)
					double pdx = x - cx0, pdy = y - cy0;
					double B = pdx * cdx + pdy * cdy + r0 * dr;
					double C = pdx * pdx + pdy * pdy - r0 * r0;
					double t;
					if (std::fabs(A) < 1e-9)
					{
						if (std::fabs(B) < 1e-12)
							return Color();
						t = C / (2 * B);
						if (r0 + t * dr < 0)
							return Color();
					}
					else
					{
						double disc = B * B - A * C;
						if (disc < 0)
							return Color();
						double s = std::sqrt(disc);
						double t1 = (B + s) / A, t2 = (B - s) / A;
						if (t1 < t2)
							std::swap(t1, t2);
						if (r0 + t1 * dr >= 0)
							t = t1;
						else if (r0 + t2 * dr >= 0)
							t = t2;
						else
							return Color();
					}
					return Sample(stops, ext, (float)t);
				});
				break;
			}
			case FT_COLR_PAINTFORMAT_SWEEP_GRADIENT:
			{
				FT_PaintSweepGradient& g = paint.u.sweep_gradient;
				std::vector<Stop> stops;
				ReadStops(g.colorline, stops);
				if (stops.empty())
					break;
				double cx = Fx(g.center.x), cy = Fx(g.center.y);
				double a0 = Fx(g.start_angle) * 180.0, a1 = Fx(g.end_angle) * 180.0;
				FT_PaintExtend ext = g.colorline.extend;
				FillEach(dst, clip, ctm, [&](double x, double y) {
					// counter-clockwise from the positive x axis, font space is y-up
					double ang = std::atan2(y - cy, x - cx) * 180.0 / c_dPi;
					if (ang < 0)
						ang += 360.0;
					float t = std::fabs(a1 - a0) < 1e-9 ? 0.0f : (float)((ang - a0) / (a1 - a0));
					return Sample(stops, ext, t);
				});
				break;
			}
			case FT_COLR_PAINTFORMAT_GLYPH:
			{
				long long bytes = (long long)m_nW * m_nH * (sizeof(float) + 1);
				if (!Reserve(bytes))
					break;
				std::vector<float> mask;
				if (!GlyphMask(paint.u.glyph.glyphID, ctm, mask))
				{
					Release(bytes);
					break;
				}
				if (clip)
					for (size_t i = 0; i < mask.size(); ++i)
						mask[i] *= clip[i];
				Draw(paint.u.glyph.paint, ctm, mask.data(), dst);
				Release(bytes);
				break;
			}
			case FT_COLR_PAINTFORMAT_COLR_GLYPH:
			{
				FT_OpaquePaint root;
				root.p = nullptr;
				if (FT_Get_Color_Glyph_Paint(m_pFace, paint.u.colr_glyph.glyphID, FT_COLOR_NO_ROOT_TRANSFORM, &root))
					Draw(root, ctm, clip, dst);
				break;
			}
			case FT_COLR_PAINTFORMAT_TRANSFORM:
			case FT_COLR_PAINTFORMAT_TRANSLATE:
			case FT_COLR_PAINTFORMAT_SCALE:
			case FT_COLR_PAINTFORMAT_ROTATE:
			case FT_COLR_PAINTFORMAT_SKEW:
			{
				Matrix m;
				FT_OpaquePaint child;
				if (ChildTransform(paint, m, child))
					Draw(child, ctm * m, clip, dst);
				break;
			}
			case FT_COLR_PAINTFORMAT_COMPOSITE:
			{
				long long bytes = 2LL * dst.size() * sizeof(Color);
				if (!Reserve(bytes))
					break;
				{
					Layer src(dst.size()), backdrop(dst.size());
					Draw(paint.u.composite.source_paint, ctm, clip, src);
					Draw(paint.u.composite.backdrop_paint, ctm, clip, backdrop);
					FT_Composite_Mode mode = paint.u.composite.composite_mode;
					for (size_t i = 0; i < dst.size(); ++i)
						Over(dst[i], Composite(src[i], backdrop[i], mode), 1.0f);
				}
				Release(bytes);
				break;
			}
			default:
				break;
			}
		}

		// Separable blend on unpremultiplied channels (W3C compositing spec)
		static float Blend(float cb, float cs, FT_Composite_Mode mode)
		{
			switch (mode)
			{
			case FT_COLR_COMPOSITE_MULTIPLY:    return cb * cs;
			case FT_COLR_COMPOSITE_SCREEN:      return cb + cs - cb * cs;
			case FT_COLR_COMPOSITE_OVERLAY:     return Blend(cs, cb, FT_COLR_COMPOSITE_HARD_LIGHT);
			case FT_COLR_COMPOSITE_DARKEN:      return std::min(cb, cs);
			case FT_COLR_COMPOSITE_LIGHTEN:     return std::max(cb, cs);
			case FT_COLR_COMPOSITE_COLOR_DODGE: return cb <= 0 ? 0 : (cs >= 1 ? 1 : std::min(1.0f, cb / (1 - cs)));
			case FT_COLR_COMPOSITE_COLOR_BURN:  return cb >= 1 ? 1 : (cs <= 0 ? 0 : 1 - std::min(1.0f, (1 - cb) / cs));
			case FT_COLR_COMPOSITE_HARD_LIGHT:  return cs <= 0.5f ? cb * 2 * cs : Blend(cb, 2 * cs - 1, FT_COLR_COMPOSITE_SCREEN);
			case FT_COLR_COMPOSITE_SOFT_LIGHT:
			{
				if (cs <= 0.5f)
					return cb - (1 - 2 * cs) * cb * (1 - cb);
				float d = cb <= 0.25f ? ((16 * cb - 12) * cb + 4) * cb : std::sqrt(cb);
				return cb + (2 * cs - 1) * (d - cb);
			}
			case FT_COLR_COMPOSITE_DIFFERENCE:  return std::fabs(cb - cs);
			case FT_COLR_COMPOSITE_EXCLUSION:   return cb + cs - 2 * cb * cs;
			default:                            return cs; // ponytail: HSL modes drawn as normal, add when a font needs them
			}
		}

		static Color Composite(const Color& s, const Color& b, FT_Composite_Mode mode)
		{
			auto pd = [&](float fa, float fb) {
				return Color{ s.r * fa + b.r * fb, s.g * fa + b.g * fb, s.b * fa + b.b * fb, s.a * fa + b.a * fb };
			};
			switch (mode)
			{
			case FT_COLR_COMPOSITE_CLEAR:     return Color();
			case FT_COLR_COMPOSITE_SRC:       return s;
			case FT_COLR_COMPOSITE_DEST:      return b;
			case FT_COLR_COMPOSITE_SRC_OVER:  return pd(1, 1 - s.a);
			case FT_COLR_COMPOSITE_DEST_OVER: return pd(1 - b.a, 1);
			case FT_COLR_COMPOSITE_SRC_IN:    return pd(b.a, 0);
			case FT_COLR_COMPOSITE_DEST_IN:   return pd(0, s.a);
			case FT_COLR_COMPOSITE_SRC_OUT:   return pd(1 - b.a, 0);
			case FT_COLR_COMPOSITE_DEST_OUT:  return pd(0, 1 - s.a);
			case FT_COLR_COMPOSITE_SRC_ATOP:  return pd(b.a, 1 - s.a);
			case FT_COLR_COMPOSITE_DEST_ATOP: return pd(1 - b.a, s.a);
			case FT_COLR_COMPOSITE_XOR:       return pd(1 - b.a, 1 - s.a);
			case FT_COLR_COMPOSITE_PLUS:
				return { std::min(1.0f, s.r + b.r), std::min(1.0f, s.g + b.g), std::min(1.0f, s.b + b.b), std::min(1.0f, s.a + b.a) };
			default:
			{
				// result = (1 - ab) * Cs + (1 - as) * Cb + as * ab * B(cb, cs), premultiplied
				float ra = s.a + b.a - s.a * b.a;
				auto ch = [&](float sp, float bp) {
					float cs = s.a > 0 ? sp / s.a : 0, cb = b.a > 0 ? bp / b.a : 0;
					return (1 - b.a) * sp + (1 - s.a) * bp + s.a * b.a * Blend(cb, cs, mode);
				};
				return { ch(s.r, b.r), ch(s.g, b.g), ch(s.b, b.b), ra };
			}
			}
		}
	};
}

bool ColrV1_HasGlyph(FT_Face face, FT_UInt gid)
{
	FT_OpaquePaint root;
	root.p = nullptr;
	return face && FT_Get_Color_Glyph_Paint(face, gid, FT_COLOR_NO_ROOT_TRANSFORM, &root);
}

bool ColrV1_RenderGlyph(FT_Face face, FT_UInt gid, unsigned int foreground, CColrV1Bitmap& out)
{
	FT_OpaquePaint root;
	root.p = nullptr;
	if (!face || !face->size || !FT_Get_Color_Glyph_Paint(face, gid, FT_COLOR_NO_ROOT_TRANSFORM, &root))
		return false;

	Matrix toDevice = DeviceMatrix(face);
	toDevice.e = toDevice.f = 0; // the bitmap is placed relative to the pen

	double xMin, yMin, xMax, yMax;
	FT_ClipBox box;
	if (FT_Get_Color_Glyph_ClipBox(face, gid, &box))
	{
		// already scaled and transformed, 26.6
		const FT_Vector pts[4] = { box.bottom_left, box.top_left, box.top_right, box.bottom_right };
		xMin = yMin = 1e30; xMax = yMax = -1e30;
		for (const FT_Vector& p : pts)
		{
			xMin = std::min(xMin, p.x / 64.0); xMax = std::max(xMax, p.x / 64.0);
			yMin = std::min(yMin, p.y / 64.0); yMax = std::max(yMax, p.y / 64.0);
		}
	}
	else
	{
		const double bx[2] = { (double)face->bbox.xMin, (double)face->bbox.xMax };
		const double by[2] = { (double)face->bbox.yMin, (double)face->bbox.yMax };
		xMin = yMin = 1e30; xMax = yMax = -1e30;
		for (double x : bx)
			for (double y : by)
			{
				double dx = toDevice.a * x + toDevice.c * y, dy = toDevice.b * x + toDevice.d * y;
				xMin = std::min(xMin, dx); xMax = std::max(xMax, dx);
				yMin = std::min(yMin, dy); yMax = std::max(yMax, dy);
			}
	}

	for (double v : { xMin, yMin, xMax, yMax })
	{
		if (!(std::fabs(v) < c_dMaxCoord))
			return false;
	}
	int left = (int)std::floor(xMin), right = (int)std::ceil(xMax);
	int bottom = (int)std::floor(yMin), top = (int)std::ceil(yMax);
	int w = right - left, h = top - bottom;
	if (w <= 0 || h <= 0 || (long long)w * h > c_nMaxPixels)
		return false;

	// Glyph masks load into a slot of our own (FT_New_GlyphSlot makes it
	// face->glyph, FT_Done_GlyphSlot restores the previous one): callers like the
	// WASM engine keep their loaded glyph, also when we refuse and they fall
	// back to the outline
	FT_GlyphSlot ownSlot = nullptr;
	if (FT_New_GlyphSlot(face, &ownSlot))
		return false;

	bool ok = false;
	CRenderer renderer(face, w, h, foreground);
	Layer layer;
	try
	{
		layer.resize((size_t)w * h);
		renderer.Draw(root, Matrix::Translate(-left, -bottom) * toDevice, nullptr, layer);
		if (!renderer.m_bAborted)
		{
			out.Rgba.resize(layer.size() * 4);
			ok = true;
		}
	}
	catch (const std::bad_alloc&)
	{
	}
	FT_Done_GlyphSlot(ownSlot);
	if (!ok)
		return false;

	out.Left = left;
	out.Top = top;
	out.Width = w;
	out.Height = h;
	for (size_t i = 0; i < layer.size(); ++i)
	{
		const Color& c = layer[i];
		float a = std::min(1.0f, std::max(0.0f, c.a));
		float k = a > 0 ? 255.0f / a : 0;
		out.Rgba[i * 4 + 0] = (unsigned char)std::lround(std::min(255.0f, c.r * k));
		out.Rgba[i * 4 + 1] = (unsigned char)std::lround(std::min(255.0f, c.g * k));
		out.Rgba[i * 4 + 2] = (unsigned char)std::lround(std::min(255.0f, c.b * k));
		out.Rgba[i * 4 + 3] = (unsigned char)std::lround(a * 255.0f);
	}
	return true;
}

namespace
{
	// Walks the paint graph like CRenderer, but only collects glyph outlines
	class CSilhouette
	{
	public:
		FT_Face m_pFace;
		std::vector<FT_Vector> m_arPoints;
		std::vector<unsigned char> m_arTags;
		std::vector<unsigned short> m_arContours;
		const void* m_arStack[c_nMaxDepth] = {};
		int m_nDepth = 0;
		int m_nNodes = 0;

		explicit CSilhouette(FT_Face face) : m_pFace(face) {}

		void Walk(const FT_OpaquePaint& opaque, const Matrix& ctm)
		{
			if (m_nDepth >= c_nMaxDepth || ++m_nNodes > c_nMaxNodes || std::find(m_arStack, m_arStack + m_nDepth, opaque.p) != m_arStack + m_nDepth)
				return;
			FT_COLR_Paint paint;
			if (!FT_Get_Paint(m_pFace, opaque, &paint))
				return;
			m_arStack[m_nDepth++] = opaque.p;

			Matrix m;
			FT_OpaquePaint child;
			switch (paint.format)
			{
			case FT_COLR_PAINTFORMAT_COLR_LAYERS:
			{
				FT_OpaquePaint layer;
				layer.p = nullptr;
				FT_LayerIterator it = paint.u.colr_layers.layer_iterator;
				while (FT_Get_Paint_Layers(m_pFace, &it, &layer))
					Walk(layer, ctm);
				break;
			}
			case FT_COLR_PAINTFORMAT_GLYPH:
				AddGlyph(paint.u.glyph.glyphID, ctm);
				break;
			case FT_COLR_PAINTFORMAT_COLR_GLYPH:
			{
				FT_OpaquePaint root;
				root.p = nullptr;
				if (FT_Get_Color_Glyph_Paint(m_pFace, paint.u.colr_glyph.glyphID, FT_COLOR_NO_ROOT_TRANSFORM, &root))
					Walk(root, ctm);
				break;
			}
			case FT_COLR_PAINTFORMAT_COMPOSITE:
				Walk(paint.u.composite.backdrop_paint, ctm);
				Walk(paint.u.composite.source_paint, ctm);
				break;
			default:
				if (ChildTransform(paint, m, child))
					Walk(child, ctm * m);
				break;
			}
			--m_nDepth;
		}

		void AddGlyph(FT_UInt gid, const Matrix& ctm)
		{
			if (FT_Load_Glyph(m_pFace, gid, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_TRANSFORM))
				return;
			const FT_Outline& o = m_pFace->glyph->outline;
			if (m_pFace->glyph->format != FT_GLYPH_FORMAT_OUTLINE || m_arPoints.size() + o.n_points > 0xFFFF)
				return;
			unsigned short first = (unsigned short)m_arPoints.size();
			for (int i = 0; i < o.n_points; ++i)
			{
				double x = (double)o.points[i].x, y = (double)o.points[i].y;
				m_arPoints.push_back({ ToPos(ctm.a * x + ctm.c * y + ctm.e), ToPos(ctm.b * x + ctm.d * y + ctm.f) });
				m_arTags.push_back(o.tags[i]);
			}
			for (int i = 0; i < o.n_contours; ++i)
				m_arContours.push_back((unsigned short)(first + o.contours[i]));
		}
	};
}

bool ColrV1_GetSilhouette(FT_Face face, FT_UInt gid, FT_Outline* out)
{
	FT_OpaquePaint root;
	root.p = nullptr;
	if (!face || !face->size || !out || !FT_Get_Color_Glyph_Paint(face, gid, FT_COLOR_NO_ROOT_TRANSFORM, &root))
		return false;

	FT_GlyphSlot ownSlot = nullptr; // see ColrV1_RenderGlyph
	if (FT_New_GlyphSlot(face, &ownSlot))
		return false;
	bool ok = false;
	try
	{
		CSilhouette silhouette(face);
		silhouette.Walk(root, DeviceMatrix(face));
		size_t n = silhouette.m_arPoints.size();
		if (n > 0 && !FT_Outline_New(face->glyph->library, (FT_UInt)n, (FT_Int)silhouette.m_arContours.size(), out))
		{
			std::copy(silhouette.m_arPoints.begin(), silhouette.m_arPoints.end(), out->points);
			std::copy(silhouette.m_arTags.begin(), silhouette.m_arTags.end(), out->tags);
			std::copy(silhouette.m_arContours.begin(), silhouette.m_arContours.end(), out->contours);
			ok = true;
		}
	}
	catch (const std::bad_alloc&)
	{
	}
	FT_Done_GlyphSlot(ownSlot);
	return ok;
}
