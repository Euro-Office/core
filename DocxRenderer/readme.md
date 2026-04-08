Logic is described in CPage::ProcessingAndRecordingOfPageData

Stage I

1. Vector graphics shapes are collected (DrawPath -> m_arShapes)
  - Current Pen and Brush are copied,
  - shape type is determined: VectorTexture or VectorGraphics
  - initial position on page is determined (before text/behind text - currently works poorly, was better when white rectangles were removed)
  - geometric parameters are set
  - graphics type is determined (Rectangle, Curve, ComplicatedFigure, NoGraphics) and subtype (LongDash, Dash, Dot, Wave)

2. Image shapes are collected (WriteImage -> m_arImages)
  - geometric parameters and shape type Picture are set

3. Letters are collected and immediately distributed across text lines. DiacriticalSymbols are collected separately (CollectTextData -> m_arTextLine, m_arDiacriticalSymbol)
  - all spaces are discarded (additional unicodes for other space types need to be added)
  - FontManager work
  - geometric parameters are set
  - style is generated or checked for existence (current Font, Brush, PickFontName, PickFontStyle are copied and analyzed)

Stage II
   All objects for the current page have been collected. Starting analysis.

1. Analyze graphics - AnalyzeCollectedShapes()
  - BuildTables(); - collect tables from shapes (in development)
  - DetermineLinesType() - convert shapes to horizontal lines based on geometry, remove processed shapes, determine resulting line type based on graphics type (Rectangle, Curve, ComplicatedFigure, NoGraphics) and subtype (LongDash, Dash, Dot, Wave). (2 nested loops m_arShapes - m_arShapes with vector sorting)

2. AnalyzeCollectedTextLines() - add properties to each character individually
  - Determine relationships between characters - FontEffects, VertAlignTypeBetweenConts, IsDuplicate (2 nested loops m_arSymbol - m_arSymbol) (remove processed symbols)
  - DetermineStrikeoutsUnderlinesHighlights() - determine relationships between graphics and characters: Strikeouts, Underlines, Highlights, FontEffect (2 nested loops m_arShapes - m_arSymbol) (remove processed shapes)
  - AddDiacriticalSymbols() - add DiacriticalSymbols
  - MergeLinesByVertAlignType() - merge lines with specific eVertAlignType
  - DeleteTextClipPage() - delete lines outside the page (1 loop m_arTextLine)
  - DetermineTextColumns() - determine text columns and add to table (in development)
  - BuildLines() - build words from characters, add spaces
  - DetermineDominantGraphics() - needed to highlight the shape that will be used for paragraph shading (2 nested loops m_arTextLine - m_arConts)
  - BuildParagraphes() - build paragraphs/shapes from text lines and add to m_arOutputObjects

Stage III ToXml
