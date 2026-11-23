The logic is described in `CPage::ProcessingAndRecordingOfPageData`.

## Stage I

1. Collect shapes with vector graphics (`DrawPath` -> `m_arShapes`)
  - Copy the current Pen and Brush,
  - determine the shape type: `VectorTexture` or `VectorGraphics`,
  - initially determine the position on the page (in front of the text / behind the text — currently this works poorly; it was better when white rectangles were removed),
  - set the geometric parameters,
  - determine the graphics type (`Rectangle`, `Curve`, `ComplicatedFigure`, `NoGraphics`) and subtype (`LongDash`, `Dash`, `Dot`, `Wave`).

2. Collect image shapes (`WriteImage` -> `m_arImages`)
  - Set the geometric parameters and the `Picture` shape type.

3. Collect letters and immediately distribute them across text lines. `DiacriticalSymbol` are collected separately (`CollectTextData` -> `m_arTextLine`, `m_arDiacriticalSymbol`)
  - Discard all spaces (additional Unicode code points for other space types will need to be added),
  - FontManager work,
  - set the geometric parameters,
  - generate or check for an existing style (the current `Font`, `Brush`, `PickFontName`, `PickFontStyle` are copied and analyzed).

## Stage II

All objects for the current page have been collected. Analysis begins.

1. Analyze the graphics — `AnalyzeCollectedShapes()`
  - `BuildTables();` — build tables from shapes (in development),
  - `DetermineLinesType()` — turn shapes into horizontal lines depending on their geometry, remove the processed shapes, and determine the resulting line type based on the graphics type (`Rectangle`, `Curve`, `ComplicatedFigure`, `NoGraphics`) and subtype (`LongDash`, `Dash`, `Dot`, `Wave`). (Two nested loops over `m_arShapes` - `m_arShapes` with vector sorting.)

2. `AnalyzeCollectedTextLines()` — add properties to each symbol individually
  - Determine the relationships between symbols — `FontEffects`, `VertAlignTypeBetweenConts`, `IsDuplicate` (two nested loops over `m_arSymbol` - `m_arSymbol`; processed symbols are removed),
  - `DetermineStrikeoutsUnderlinesHighlights()` — determine the relationships between graphics and symbols: `Strikeouts`, `Underlines`, `Highlights`, `FontEffect` (two nested loops over `m_arShapes` - `m_arSymbol`; processed shapes are removed),
  - `AddDiacriticalSymbols()` — add `DiacriticalSymbol`,
  - `MergeLinesByVertAlignType()` — merge lines with a specific `eVertAlignType`,
  - `DeleteTextClipPage()` — remove lines outside the page (one loop over `m_arTextLine`),
  - `DetermineTextColumns()` — determine text columns and add them to the table (in development),
  - `BuildLines()` — assemble words from symbols, add spaces,
  - `DetermineDominantGraphics()` — needed to select the shape that will be used as the paragraph shading (two nested loops over `m_arTextLine` - `m_arConts`),
  - `BuildParagraphes()` — assemble paragraphs/shapes from text lines and add them to `m_arOutputObjects`.

## Stage III — ToXml
