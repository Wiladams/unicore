Script definition/front end
        |
        v
ScriptShapingIRBuilder
        |
        | finalize()
        v
ScriptShapingIR
        |
        v
ScriptShapingIRExecutor
        |
        +---- scalar-domain operations
        |
        +---- GSUB feature stages
        |
        +---- nominal metrics boundary
        |
        +---- GPOS feature stages
        |
        v
ShapedGlyphBuffer