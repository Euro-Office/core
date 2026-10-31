// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

/*
 * Tests for the JS engine embedding of doctrenderer (js_internal/v8).
 *
 * They go through the NSJSBase API that core uses, and cover what an engine
 * update can break without failing to compile: value conversion, exceptions,
 * typed arrays over native memory, the code cache, embedded native objects and
 * their lifetime, several contexts and threads, and the optimizing compilers
 * (V8 14 crashed in TurboFan while the interpreter was fine).
 */

#include "gtest/gtest.h"

#include "js_internal/js_base.h"
#include "js_internal/js_embed.h"
#include "../../../common/File.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

using namespace NSJSBase;

// An embedded native object counting its instances, to check that every one
// created from JS is destroyed exactly once.
class CCountingEmbed : public CJSEmbedObject
{
public:
	static std::atomic<int> s_constructed;
	static std::atomic<int> s_destroyed;

	CCountingEmbed() { ++s_constructed; }
	~CCountingEmbed() { ++s_destroyed; }

	virtual void* getObject() override { return nullptr; }

	JSSmart<CJSValue> add(JSSmart<CJSValue> a, JSSmart<CJSValue> b)
	{
		return CJSContext::createInt(a->toInt32() + b->toInt32());
	}
	JSSmart<CJSValue> echo(JSSmart<CJSValue> value)
	{
		return CJSContext::createString(value->toStringW());
	}
	JSSmart<CJSValue> bytes(JSSmart<CJSValue> count)
	{
		// a typed array over memory V8 takes ownership of
		int n = count->toInt32();
		BYTE* data = NSAllocator::Alloc((size_t)n);
		for (int i = 0; i < n; ++i)
			data[i] = (BYTE)(i & 0xFF);
		return CJSContext::createUint8Array(data, n, false);
	}

	DECLARE_EMBED_METHODS
};

std::atomic<int> CCountingEmbed::s_constructed{0};
std::atomic<int> CCountingEmbed::s_destroyed{0};

class CCountingEmbedAdapter : public CJSEmbedObjectAdapterV8
{
public:
	virtual std::vector<std::string> getMethodNames() override
	{
		return { "add", "echo", "bytes" };
	}

	virtual void initFunctions(CJSEmbedObject* pNativeObjBase) override
	{
		CCountingEmbed* pNativeObj = static_cast<CCountingEmbed*>(pNativeObjBase);
		m_functions = std::vector<EmbedFunctionType> {
			[pNativeObj](CJSFunctionArguments* args) { return pNativeObj->add(args->Get(0), args->Get(1)); },
			[pNativeObj](CJSFunctionArguments* args) { return pNativeObj->echo(args->Get(0)); },
			[pNativeObj](CJSFunctionArguments* args) { return pNativeObj->bytes(args->Get(0)); },
		};
	}
};

CJSEmbedObjectAdapterBase* CCountingEmbed::getAdapter()
{
	if (m_pAdapter == nullptr)
		m_pAdapter = new CCountingEmbedAdapter();
	return m_pAdapter;
}

std::string CCountingEmbed::getName() { return "CCountingEmbed"; }

CJSEmbedObject* CCountingEmbed::getCreator()
{
	return new CCountingEmbed();
}

// The same class, registered as not creatable from JS
class CHiddenEmbed : public CCountingEmbed
{
public:
	static std::string getName() { return "CHiddenEmbed"; }
	static CJSEmbedObject* getCreator() { return new CHiddenEmbed(); }
};

class CEngineTest : public testing::Test
{
public:
	static void SetUpTestSuite()
	{
		CJSContext::Embed<CCountingEmbed>();
		CJSContext::Embed<CHiddenEmbed>(false);
	}

	void SetUp() override
	{
		m_pContext = new CJSContext();
		m_pContext->Enter();
	}

	void TearDown() override
	{
		m_pContext->Exit();
	}

	JSSmart<CJSValue> run(const std::string& script)
	{
		return m_pContext->runScript(script);
	}

public:
	JSSmart<CJSContext> m_pContext;
};

// Value conversion

TEST_F(CEngineTest, primitive_results)
{
	EXPECT_EQ(run("6 * 7")->toInt32(), 42);
	EXPECT_EQ(run("-1 >>> 0")->toUInt32(), 4294967295u);
	EXPECT_DOUBLE_EQ(run("0.1 + 0.2")->toDouble(), 0.1 + 0.2);
	EXPECT_TRUE(run("1 < 2")->toBool());
	EXPECT_TRUE(run("null")->isNull());
	EXPECT_TRUE(run("undefined")->isUndefined());
	EXPECT_EQ(run("'abc' + 1")->toStringA(), "abc1");
	EXPECT_TRUE(run("[1, 2]")->isArray());
	EXPECT_TRUE(run("(function() {})")->isFunction());
	EXPECT_TRUE(run("({})")->isObject());
}

TEST_F(CEngineTest, unicode_strings_round_trip)
{
	std::wstring text = L"Grüße, Ελληνικά, 日本語, \U0001F600";
	JSSmart<CJSObject> global = m_pContext->GetGlobal();
	global->set("text", CJSContext::createString(text));

	EXPECT_EQ(run("text")->toStringW(), text);
	// JS counts UTF-16 code units, the emoji is a surrogate pair
	EXPECT_EQ(run("text.length")->toInt32(), (int)std::u16string(u"Grüße, Ελληνικά, 日本語, \U0001F600").length());
	EXPECT_EQ(run("text.codePointAt(text.length - 2)")->toInt32(), 0x1F600);
	EXPECT_EQ(run("'Ünïcödé'.toUpperCase()")->toStringW(), L"ÜNÏCÖDÉ");
}

TEST_F(CEngineTest, strings_with_embedded_nul)
{
	std::string withNul("a\0b", 3);
	JSSmart<CJSObject> global = m_pContext->GetGlobal();
	global->set("s", CJSContext::createString(withNul.c_str(), (int)withNul.length()));
	EXPECT_EQ(run("s.length")->toInt32(), 3);
	EXPECT_EQ(run("s")->toStringA(), withNul);
}

TEST_F(CEngineTest, objects_and_arrays_from_native)
{
	JSSmart<CJSObject> obj = CJSContext::createObject();
	obj->set("i", 7);
	obj->set("d", 2.5);
	obj->set("s", CJSContext::createString("str"));

	JSSmart<CJSArray> arr = CJSContext::createArray(0);
	arr->add_int(1);
	arr->add_double(1.5);
	arr->add_bool(true);
	arr->add_stringa("x");
	arr->add_null();
	arr->add_undefined();
	obj->set("arr", arr->toValue());

	m_pContext->GetGlobal()->set("obj", obj);
	EXPECT_EQ(run("JSON.stringify(obj)")->toStringA(), "{\"i\":7,\"d\":2.5,\"s\":\"str\",\"arr\":[1,1.5,true,\"x\",null,null]}");
	EXPECT_EQ(run("obj.arr.length")->toInt32(), 6);
}

TEST_F(CEngineTest, call_js_functions_from_native)
{
	run("var calc = { mul: function(a, b) { return a * b; } }; function concat(a, b) { return a + b; }");

	JSSmart<CJSValue> args[2] = { CJSContext::createInt(6), CJSContext::createInt(7) };
	JSSmart<CJSObject> calc = m_pContext->GetGlobal()->get("calc")->toObject();
	EXPECT_EQ(calc->call_func("mul", 2, args)->toInt32(), 42);

	JSSmart<CJSFunction> concat = m_pContext->GetGlobal()->get("concat")->toFunction();
	JSSmart<CJSValue> strArgs[2] = { CJSContext::createString("ab"), CJSContext::createString("cd") };
	JSSmart<CJSValue> recv = CJSContext::createUndefined();
	EXPECT_EQ(concat->Call(recv.GetPointer(), 2, strArgs)->toStringA(), "abcd");

	// calling something that isn't a function returns an empty value, no crash
	EXPECT_TRUE(calc->call_func("missing")->isEmpty());
}

// Language features sdkjs relies on or may start to use

TEST_F(CEngineTest, modern_language_features)
{
	const char* script = R"JS(
		class Base { #secret = 41; get value() { return this.#secret + 1; } static make() { return new this(); } }
		class Derived extends Base {}
		const results = [
			Derived.make().value === 42,
			({ a: { b: null } }).a.b?.c === undefined,
			(null ?? 'fallback') === 'fallback',
			(2n ** 64n).toString() === '18446744073709551616',
			[1, 2, 3].at(-1) === 3,
			Object.hasOwn({ k: 1 }, 'k'),
			'a-b-c'.replaceAll('-', '+') === 'a+b+c',
			Object.fromEntries(new Map([['x', 1]])).x === 1,
			[[1, [2]], [3]].flat(Infinity).length === 3,
			/(?<year>\d{4})/.exec('in 2026').groups.year === '2026',
			typeof Symbol.iterator === 'symbol',
			new Set([1, 1, 2]).size === 2,
			typeof WeakRef === 'function',
			Array.from({ length: 3 }, (_, i) => i * 2).join() === '0,2,4',
			typeof Promise.allSettled === 'function',
		];
		results.map((ok, i) => ok ? '' : i).filter(x => x !== '').join(',')
	)JS";
	EXPECT_EQ(run(script)->toStringA(), "") << "failed feature checks (indices)";
}

TEST_F(CEngineTest, date_and_math)
{
	EXPECT_EQ(run("new Date(Date.UTC(2026, 9, 30, 12, 0, 0)).toISOString()")->toStringA(), "2026-10-30T12:00:00.000Z");
	EXPECT_EQ(run("Math.round(Math.sin(Math.PI / 2) * 1000)")->toInt32(), 1000);
	EXPECT_EQ(run("(1234.5678).toFixed(2)")->toStringA(), "1234.57");
	EXPECT_EQ(run("parseInt('ff', 16)")->toInt32(), 255);
}

// Exceptions must be reported to the caller and leave the context usable

TEST_F(CEngineTest, syntax_error_is_reported_not_fatal)
{
	JSSmart<CJSTryCatch> tryCatch = m_pContext->GetExceptions();
	m_pContext->runScript("this is not javascript", tryCatch);
	EXPECT_TRUE(tryCatch->Check());

	EXPECT_EQ(run("1 + 1")->toInt32(), 2) << "context still usable";
}

TEST_F(CEngineTest, runtime_error_is_reported_not_fatal)
{
	JSSmart<CJSTryCatch> tryCatch = m_pContext->GetExceptions();
	JSSmart<CJSValue> result = m_pContext->runScript("(function() { throw new TypeError('boom'); })()", tryCatch);
	EXPECT_TRUE(tryCatch->Check());
	EXPECT_TRUE(result->isEmpty());

	EXPECT_EQ(run("'still' + ' alive'")->toStringA(), "still alive");
}

TEST_F(CEngineTest, exceptions_caught_in_js_are_not_reported)
{
	JSSmart<CJSTryCatch> tryCatch = m_pContext->GetExceptions();
	JSSmart<CJSValue> result = m_pContext->runScript("try { null.x; } catch (e) { e instanceof TypeError }", tryCatch);
	EXPECT_FALSE(tryCatch->Check());
	EXPECT_TRUE(result->toBool());
}

TEST_F(CEngineTest, deep_recursion_is_a_range_error)
{
	JSSmart<CJSTryCatch> tryCatch = m_pContext->GetExceptions();
	JSSmart<CJSValue> result = m_pContext->runScript(
		"(function() { function f(n) { return f(n + 1) + 1; } try { f(0); } catch (e) { return e instanceof RangeError; } })()", tryCatch);
	EXPECT_FALSE(tryCatch->Check());
	EXPECT_TRUE(result->toBool());
}

// JSON

TEST_F(CEngineTest, json_round_trip)
{
	const char* json = "{\"name\":\"Grüße\",\"list\":[1,2.5,true,null,{\"nested\":\"x\"}],\"empty\":{}}";
	JSSmart<CJSValue> value = m_pContext->JSON_Parse(json);
	ASSERT_TRUE(value->isObject());
	EXPECT_EQ(m_pContext->JSON_Stringify(value), json);
}

TEST_F(CEngineTest, json_parse_invalid_is_undefined)
{
	JSSmart<CJSValue> value = m_pContext->JSON_Parse("{not json");
	EXPECT_TRUE(value->isUndefined());
	EXPECT_EQ(run("2 + 2")->toInt32(), 4) << "context still usable";
}

// Typed arrays over native memory (needs V8 without the sandbox)

TEST_F(CEngineTest, externalized_typed_array_shares_native_memory)
{
	// externalized: the memory stays ours
	std::vector<BYTE> memory(256);
	for (size_t i = 0; i < memory.size(); ++i)
		memory[i] = (BYTE)i;

	JSSmart<CJSTypedArray> array = CJSContext::createUint8Array(memory.data(), (int)memory.size(), true);
	m_pContext->GetGlobal()->set("bytes", array->toValue());

	EXPECT_EQ(run("bytes.length")->toInt32(), 256);
	EXPECT_EQ(run("bytes.reduce((a, b) => a + b, 0)")->toInt32(), 255 * 256 / 2);

	// writes from JS land in our memory and vice versa
	run("bytes[0] = 200; bytes[255] = 7;");
	EXPECT_EQ(memory[0], 200);
	EXPECT_EQ(memory[255], 7);
	memory[1] = 99;
	EXPECT_EQ(run("bytes[1]")->toInt32(), 99);

	CJSDataBuffer buffer = array->getData();
	EXPECT_EQ(buffer.Data, memory.data());
	EXPECT_EQ(buffer.Len, memory.size());
}

TEST_F(CEngineTest, owned_typed_array_memory_is_released_by_engine)
{
	// not externalized: V8 frees the memory with NSAllocator::Free
	for (int round = 0; round < 50; ++round)
	{
		CJSLocalScope scope;
		BYTE* data = NSAllocator::Alloc(1024 * 1024);
		std::memset(data, round, 1024 * 1024);
		JSSmart<CJSTypedArray> array = CJSContext::createUint8Array(data, 1024 * 1024, false);
		m_pContext->GetGlobal()->set("big", array->toValue());
		EXPECT_EQ(run("big[1024 * 1024 - 1]")->toInt32(), round);
	}
	run("big = null");
}

TEST_F(CEngineTest, typed_array_from_js_to_native)
{
	JSSmart<CJSTypedArray> array = run("new Uint8Array([10, 20, 30, 40])")->toTypedArray();
	ASSERT_EQ(array->getCount(), 4);
	CJSDataBuffer buffer = array->getData();
	ASSERT_EQ(buffer.Len, 4u);
	EXPECT_EQ(buffer.Data[0], 10);
	EXPECT_EQ(buffer.Data[3], 40);
}

TEST_F(CEngineTest, detached_typed_array_is_empty_in_js)
{
	std::vector<BYTE> memory(64, 1);
	JSSmart<CJSTypedArray> array = CJSContext::createUint8Array(memory.data(), (int)memory.size(), true);
	m_pContext->GetGlobal()->set("bytes", array->toValue());
	EXPECT_EQ(run("bytes.length")->toInt32(), 64);

	array->Detach();
	EXPECT_EQ(run("bytes.length")->toInt32(), 0);
	EXPECT_TRUE(run("bytes.buffer.detached")->toBool());
}

TEST_F(CEngineTest, large_allocations)
{
	// a few hundred MB, as large documents need
	EXPECT_EQ(run("(function() { const a = new Uint8Array(256 * 1024 * 1024); a[a.length - 1] = 5; return a[a.length - 1]; })()")->toInt32(), 5);
	EXPECT_EQ(run("(function() { const parts = []; for (let i = 0; i < 200000; i++) parts.push('line ' + i); return parts.join('\\n').length > 2000000; })()")->toBool(), true);
}

// Code cache (sdk-all.cache)

class CCodeCacheTest : public CEngineTest
{
public:
	void SetUp() override
	{
		CEngineTest::SetUp();
		m_dir = std::filesystem::temp_directory_path() / ("doctrenderer_cache_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + "_" + ::testing::UnitTest::GetInstance()->current_test_info()->name());
		std::filesystem::remove_all(m_dir);
		std::filesystem::create_directories(m_dir);
	}
	void TearDown() override
	{
		std::filesystem::remove_all(m_dir);
		CEngineTest::TearDown();
	}

	std::wstring scriptPath() { return (m_dir / "script.js").wstring(); }
	std::filesystem::path cachePath() { return m_dir / "script.cache"; }

	static std::string script()
	{
		std::string s = "var table = [];";
		for (int i = 0; i < 300; ++i)
			s += "function f" + std::to_string(i) + "(x) { return x * " + std::to_string(i) + " + " + std::to_string(i) + "; } table.push(f" + std::to_string(i) + ");";
		s += "table.reduce((sum, f, i) => sum + f(i), 0);";
		return s;
	}
	static int expected()
	{
		int sum = 0;
		for (int i = 0; i < 300; ++i)
			sum += i * i + i;
		return sum;
	}

	int runWithCache()
	{
		JSSmart<CJSContext> context = new CJSContext();
		CJSContextScope scope(context);
		JSSmart<CJSTryCatch> tryCatch = context->GetExceptions();
		JSSmart<CJSValue> result = context->runScript(script(), tryCatch, scriptPath());
		EXPECT_FALSE(tryCatch->Check());
		return result->toInt32();
	}

public:
	std::filesystem::path m_dir;
};

TEST_F(CCodeCacheTest, cache_is_created_and_consumed)
{
	EXPECT_EQ(runWithCache(), expected());
	ASSERT_TRUE(std::filesystem::exists(cachePath()));
	EXPECT_GT(std::filesystem::file_size(cachePath()), 0u);

	EXPECT_EQ(runWithCache(), expected()) << "result from the cached code";
}

TEST_F(CCodeCacheTest, corrupt_cache_is_rejected_not_fatal)
{
	// e.g. a cache written by another V8 version
	{
		FILE* file = std::fopen(cachePath().string().c_str(), "wb");
		ASSERT_NE(file, nullptr);
		for (int i = 0; i < 4096; ++i)
			std::fputc((i * 31) & 0xFF, file);
		std::fclose(file);
	}
	EXPECT_EQ(runWithCache(), expected());
}

TEST_F(CCodeCacheTest, empty_cache_is_rejected_not_fatal)
{
	{
		FILE* file = std::fopen(cachePath().string().c_str(), "wb");
		ASSERT_NE(file, nullptr);
		std::fclose(file);
	}
	EXPECT_EQ(runWithCache(), expected());
}

TEST_F(CCodeCacheTest, syntax_error_creates_no_cache)
{
	JSSmart<CJSContext> context = new CJSContext();
	CJSContextScope scope(context);
	JSSmart<CJSTryCatch> tryCatch = context->GetExceptions();
	context->runScript("function (", tryCatch, scriptPath());
	EXPECT_TRUE(tryCatch->Check());
	EXPECT_FALSE(std::filesystem::exists(cachePath()));
}

// Embedded native objects

TEST_F(CEngineTest, embedded_object_methods)
{
	EXPECT_EQ(run("var o = CreateEmbedObject('CCountingEmbed'); o.add(40, 2)")->toInt32(), 42);
	EXPECT_EQ(run("o.echo('Grüße')")->toStringW(), L"Grüße");
	EXPECT_EQ(run("var b = o.bytes(300); b.length + b[255] + b[299]")->toInt32(), 300 + 255 + 43);

	JSSmart<CJSObject> fromNative = CJSContext::createEmbedObject("CCountingEmbed");
	ASSERT_TRUE(fromNative.is_init());
	EXPECT_NE(dynamic_cast<CCountingEmbed*>(fromNative->getNative()), nullptr);
}

TEST_F(CEngineTest, unknown_or_hidden_embedded_objects_are_undefined)
{
	EXPECT_TRUE(run("CreateEmbedObject('NoSuchEmbed')")->isUndefined());
	EXPECT_TRUE(run("CreateEmbedObject('CHiddenEmbed')")->isUndefined());
	EXPECT_TRUE(run("CreateEmbedObject()")->isUndefined());
}

TEST_F(CEngineTest, free_embedded_object_destroys_native)
{
	int destroyed = CCountingEmbed::s_destroyed;
	run("var o = CreateEmbedObject('CCountingEmbed'); FreeEmbedObject(o);");
	EXPECT_EQ(CCountingEmbed::s_destroyed, destroyed + 1);
}

TEST(CEmbedLifetimeTest, every_native_object_is_destroyed_exactly_once)
{
	CJSContext::Embed<CCountingEmbed>();
	int constructed = CCountingEmbed::s_constructed;
	int destroyed = CCountingEmbed::s_destroyed;
	{
		JSSmart<CJSContext> context = new CJSContext();
		CJSContextScope scope(context);
		context->runScript(
			// kept alive until the context is disposed
			"var kept = []; for (let i = 0; i < 100; i++) kept.push(CreateEmbedObject('CCountingEmbed'));"
			// freed explicitly
			"for (let i = 0; i < 50; i++) FreeEmbedObject(CreateEmbedObject('CCountingEmbed'));"
			// garbage, collected by V8 or destroyed on disposal
			"for (let i = 0; i < 20000; i++) { CreateEmbedObject('CCountingEmbed').add(i, 1); new Array(100).fill(i); }");
	}
	int created = CCountingEmbed::s_constructed - constructed;
	EXPECT_EQ(created, 100 + 50 + 20000);
	EXPECT_EQ(CCountingEmbed::s_destroyed - destroyed, created);
}

// Several contexts and threads

TEST(CContextsTest, contexts_are_isolated)
{
	JSSmart<CJSContext> context1 = new CJSContext();
	JSSmart<CJSContext> context2 = new CJSContext();
	{
		CJSContextScope scope(context1);
		context1->runScript("var shared = 'one';");
	}
	{
		CJSContextScope scope(context2);
		EXPECT_EQ(context2->runScript("typeof shared")->toStringA(), "undefined");
		context2->runScript("var shared = 'two';");
	}
	{
		CJSContextScope scope(context1);
		EXPECT_EQ(context1->runScript("shared")->toStringA(), "one");
	}
}

TEST(CContextsTest, nested_enter_of_same_context)
{
	JSSmart<CJSContext> context = new CJSContext();
	context->Enter();
	{
		CJSContextScope scope(context);
		CJSLocalScope localScope;
		context->runScript("var inner = 1;");
	}
	EXPECT_EQ(context->runScript("inner + 1")->toInt32(), 2);
	context->Exit();
}

TEST(CContextsTest, many_contexts_created_and_disposed)
{
	CJSContext::Embed<CCountingEmbed>();
	for (int i = 0; i < 25; ++i)
	{
		JSSmart<CJSContext> context = new CJSContext();
		CJSContextScope scope(context);
		JSSmart<CJSValue> result = context->runScript(
			"var o = CreateEmbedObject('CCountingEmbed'); var data = new Uint8Array(1 << 20); data[5] = " + std::to_string(i) + "; o.add(data[5], 1)");
		EXPECT_EQ(result->toInt32(), i + 1);
	}
}

TEST(CContextsTest, contexts_on_several_threads)
{
	CJSContext::Embed<CCountingEmbed>();
	const int threadCount = 4;
	std::vector<int> results(threadCount, -1);
	std::vector<std::thread> threads;
	for (int t = 0; t < threadCount; ++t)
	{
		threads.emplace_back([t, &results]() {
			JSSmart<CJSContext> context = new CJSContext();
			CJSContextScope scope(context);
			JSSmart<CJSValue> result = context->runScript(
				"(function() { let sum = 0; const o = CreateEmbedObject('CCountingEmbed');"
				"  for (let i = 0; i < 300000; i++) sum = (sum + o.add(i % 7, " + std::to_string(t) + ")) % 1000003;"
				"  return sum; })()");
			results[t] = result->toInt32();
		});
	}
	for (std::thread& thread : threads)
		thread.join();

	for (int t = 0; t < threadCount; ++t)
	{
		long long sum = 0;
		for (int i = 0; i < 300000; ++i)
			sum = (sum + i % 7 + t) % 1000003;
		EXPECT_EQ(results[t], (int)sum) << "thread " << t;
	}
}

// Optimizing compilers: run code hot enough for V8 to optimize it (Maglev,
// TurboFan, also on background threads), deoptimize it and check the results.

TEST_F(CEngineTest, hot_numeric_code)
{
	const char* script = R"JS(
		function dot(a, b) { let s = 0; for (let i = 0; i < a.length; i++) s += a[i] * b[i]; return s; }
		const a = new Float64Array(1000), b = new Float64Array(1000);
		for (let i = 0; i < 1000; i++) { a[i] = i; b[i] = 2; }
		let total = 0;
		for (let round = 0; round < 3000; round++) total += dot(a, b);
		total
	)JS";
	EXPECT_DOUBLE_EQ(run(script)->toDouble(), 3000.0 * 999 * 1000);
}

TEST_F(CEngineTest, hot_object_code_with_deoptimization)
{
	const char* script = R"JS(
		class Point { constructor(x, y) { this.x = x; this.y = y; } len2() { return this.x * this.x + this.y * this.y; } }
		function sumLen(points) { let s = 0; for (const p of points) s += p.len2(); return s; }
		const points = [];
		for (let i = 0; i < 1000; i++) points.push(new Point(i % 10, 1));
		let total = 0;
		for (let round = 0; round < 2000; round++) total += sumLen(points);
		// change the shapes and types the optimized code was built for
		points.push({ len2() { return 0.5; } });
		points.push(new Point('3', 4));
		total += sumLen(points);
		total
	)JS";
	// 2001 rounds over the 1000 points, + 0.5 + ('3' * '3' + 16)
	double perRound = 0;
	for (int i = 0; i < 1000; ++i)
		perRound += (i % 10) * (i % 10) + 1;
	EXPECT_DOUBLE_EQ(run(script)->toDouble(), 2001 * perRound + 0.5 + 25);
}

TEST_F(CEngineTest, hot_string_and_regexp_code)
{
	const char* script = R"JS(
		function words(text) { const m = text.match(/[A-Za-zÄÖÜäöüß]+/g); return m ? m.length : 0; }
		function build(i) { return 'Zeile ' + i + ': Grüße aus dem Büro, ' + (i % 3 ? 'ja' : 'nein'); }
		let count = 0, chars = 0;
		for (let i = 0; i < 100000; i++) { const s = build(i); count += words(s); chars += s.length; }
		count + ':' + (chars > 0)
	)JS";
	// "Zeile", "Grüße", "aus", "dem", "Büro", "ja"/"nein" = 6 words per line
	EXPECT_EQ(run(script)->toStringA(), "600000:true");
}

TEST_F(CEngineTest, hot_code_calling_embedded_object)
{
	const char* script = R"JS(
		const o = CreateEmbedObject('CCountingEmbed');
		let sum = 0;
		for (let i = 0; i < 200000; i++) sum += o.add(i & 15, 1);
		sum
	)JS";
	long long expectedSum = 0;
	for (int i = 0; i < 200000; ++i)
		expectedSum += (i & 15) + 1;
	EXPECT_EQ(run(script)->toInt32(), (int)expectedSum);
}

TEST_F(CEngineTest, hot_closures_and_try_catch)
{
	const char* script = R"JS(
		function makeCounter(step) { let n = 0; return () => { n += step; if (n % 1000 === 0) throw n; return n; }; }
		let caught = 0, last = 0;
		const counters = [makeCounter(1), makeCounter(2), makeCounter(5)];
		for (let i = 0; i < 300000; i++) {
			try { last = counters[i % 3](); } catch (e) { caught++; }
		}
		caught
	)JS";
	// counter k is called 100000 times; it throws whenever its value hits a multiple of 1000
	int caught = 100000 / 1000 + (2 * 100000) / 1000 + (5 * 100000) / 1000;
	EXPECT_EQ(run(script)->toInt32(), caught);
}
