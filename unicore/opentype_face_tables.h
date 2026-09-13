// opentype_face_tables.h
#pragma once

#include "font_face.h"
#include "opentype_types.h"

namespace waavs
{
    // ====================================================================
    // openTypeTableProvider
    //
    // Query a format-neutral FontFace for its OpenType table interface.
    //
    // Non-OpenType font providers simply return nullptr.
    // ====================================================================

    [[nodiscard]]
    static inline const IProvideOpenTypeTables* openTypeTableProvider(const FontFace& face) noexcept
    {
        if (!face)
            return nullptr;

        return dynamic_cast<const IProvideOpenTypeTables*>(face.operator->());
    }


    // ====================================================================
    // openTypeTable
    //
    // Convenience lookup for one OpenType table.
    // ====================================================================

    [[nodiscard]]
    static inline const TableRecord* openTypeTable(const FontFace& face, Tag tag) noexcept
    {
        const IProvideOpenTypeTables* tables = openTypeTableProvider(face);
        return tables ? tables->getTable(tag) : nullptr;
    }

} // namespace waavs