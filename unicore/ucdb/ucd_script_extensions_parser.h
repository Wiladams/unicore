// ucd_script_extensions_parser.h

#pragma once

#include <cstddef>
#include <cstdint>

#include "core_nametable.h"
#include "core_openhashmap.h"

#include "ucd_parser.h"
#include "ucd_property_value_aliases_parser.h"
#include "unicode_database_builder.h"
#include "unicode_script.h"
#include "unicode_script_extensions_data.h"
#include "unicode_script_set.h"


namespace waavs
{
    // ========================================================================
    // UCDScriptExtensionsParseError
    // ========================================================================

    enum class UCDScriptExtensionsParseError : uint8_t
    {
        None = 0,

        ExistingScriptExtensions,
        MissingAliases,
        MissingScripts,
        InvalidAlias,
        StorageFailed,

        InvalidRange,
        MissingScriptList,
        UnexpectedField,
        NameInternFailed,
        UnknownScriptName,
        DuplicateScriptName,
        EmptyScriptSet,
        InvalidRangeOrder,

        ScriptSetAddFailed,
        ScriptRangeAddFailed
    };


    // ========================================================================
    // UCDScriptExtensionsParseResult
    // ========================================================================

    struct UCDScriptExtensionsParseResult
    {
        UCDScriptExtensionsParseError error{
            UCDScriptExtensionsParseError::None
        };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };
        uint32_t uniqueSetCount{ 0 };

        size_t explicitCodePoints{ 0 };


        [[nodiscard]] bool success() const noexcept
        {
            return error == UCDScriptExtensionsParseError::None;
        }


        explicit operator bool() const noexcept
        {
            return success();
        }
    };


    // ========================================================================
    // Error text
    // ========================================================================

    static inline const char* ucdScriptExtensionsParseErrorString(
        UCDScriptExtensionsParseError error) noexcept
    {
        switch (error)
        {
        case UCDScriptExtensionsParseError::None:
            return "no error";

        case UCDScriptExtensionsParseError::ExistingScriptExtensions:
            return "database already contains Script_Extensions data";

        case UCDScriptExtensionsParseError::MissingAliases:
            return "Script value alias table is empty";

        case UCDScriptExtensionsParseError::MissingScripts:
            return "database Script records have not been generated";

        case UCDScriptExtensionsParseError::InvalidAlias:
            return "invalid Script alias record";

        case UCDScriptExtensionsParseError::StorageFailed:
            return "failed to allocate Script_Extensions parser storage";

        case UCDScriptExtensionsParseError::InvalidRange:
            return "invalid Script_Extensions code-point range";

        case UCDScriptExtensionsParseError::MissingScriptList:
            return "missing Script_Extensions Script list";

        case UCDScriptExtensionsParseError::UnexpectedField:
            return "unexpected extra field in ScriptExtensions.txt record";

        case UCDScriptExtensionsParseError::NameInternFailed:
            return "failed to intern Script_Extensions Script name";

        case UCDScriptExtensionsParseError::UnknownScriptName:
            return "Script_Extensions name is not present in PropertyValueAliases.txt";

        case UCDScriptExtensionsParseError::DuplicateScriptName:
            return "duplicate Script name in Script_Extensions set";

        case UCDScriptExtensionsParseError::EmptyScriptSet:
            return "Script_Extensions record contains an empty Script set";

        case UCDScriptExtensionsParseError::InvalidRangeOrder:
            return "ScriptExtensions.txt ranges are not ordered or overlap";

        case UCDScriptExtensionsParseError::ScriptSetAddFailed:
            return "failed to add Script_Extensions Script set";

        case UCDScriptExtensionsParseError::ScriptRangeAddFailed:
            return "failed to add Script_Extensions range";
        }

        return "unknown ScriptExtensions.txt parser error";
    }


    namespace ucd_detail
    {
        // ====================================================================
        // scriptExtensionIsSpace
        //
        // ScriptExtensions.txt separates Script aliases with ASCII whitespace.
        // UCDParser has already removed comments and line terminators.
        // ====================================================================

        static inline bool scriptExtensionIsSpace(uint8_t c) noexcept
        {
            return c == ' ' || c == '\t';
        }


        // ====================================================================
        // readScriptExtensionName
        //
        // Read one whitespace-separated Script short name from a field.
        //
        // Examples:
        //
        //      Latn
        //      Arab Syrc
        //      Hani Hira Kana
        //
        // source is advanced past the returned token and any leading
        // whitespace preceding it.
        // ====================================================================

        static inline bool readScriptExtensionName(
            ByteSpan& source,
            ByteSpan& outName) noexcept
        {
            outName.reset();


            while (source &&
                scriptExtensionIsSpace(*source))
            {
                ++source;
            }


            if (!source)
                return false;


            const uint8_t* first =
                source.begin();


            while (source &&
                !scriptExtensionIsSpace(*source))
            {
                ++source;
            }


            outName.resetPointers(
                first,
                source.begin());


            return !!outName;
        }
    }


    // ========================================================================
    // ucdParseScriptExtensions
    //
    // Parse ScriptExtensions.txt and append the explicit Script_Extensions
    // ranges and deduplicated Script sets to UnicodeDatabaseBuilder.
    //
    // Expected meaningful line syntax:
    //
    //      range ; short-script-name short-script-name ...
    //
    // Examples:
    //
    //      00B7       ; Avst Cari Copt Dupl Elba Geor Glag Goth Grek Hani
    //      0300       ; Cher Copt Cyrl Grek Latn Perm Sunu Tale
    //      060C       ; Arab Gara Nkoo Rohg Syrc Thaa Yezi
    //
    // ScriptExtensions.txt uses the preferred short Script aliases from
    // PropertyValueAliases.txt.
    //
    //
    // Parsing:
    //
    //      ScriptExtensions.txt
    //              |
    //              v
    //      code-point range
    //              |
    //              v
    //      short Script aliases
    //              |
    //              v
    //      UnicodeScriptSet
    //              |
    //              v
    //      database.addScriptSet()
    //              |
    //              v
    //      UnicodeScriptSetIndex
    //              |
    //              v
    //      database.addScriptExtensionRange()
    //
    //
    // Only explicit ScriptExtensions.txt ranges are stored.
    //
    // Code points not present in ScriptExtensions.txt are deliberately not
    // added here. Their runtime Script_Extensions value is:
    //
    //      { Script(cp) }
    //
    // ========================================================================

    static inline bool ucdParseScriptExtensions(
        const ByteSpan& source,
        const UCDScriptValueAliases& aliases,
        UnicodeDatabaseBuilder& database,
        UCDScriptExtensionsParseResult& outResult)
    {
        outResult = {};


        // --------------------------------------------------------------------
        // Script_Extensions should be generated once per database.
        // --------------------------------------------------------------------

        if (database.scriptSetCount() != 0 ||
            database.scriptExtensionRangeCount() != 0)
        {
            outResult.error =
                UCDScriptExtensionsParseError::ExistingScriptExtensions;

            return false;
        }


        // --------------------------------------------------------------------
        // Script aliases are required because ScriptExtensions.txt uses the
        // preferred short Script names.
        // --------------------------------------------------------------------

        if (aliases.empty())
        {
            outResult.error =
                UCDScriptExtensionsParseError::MissingAliases;

            return false;
        }


        if (aliases.size() >
            static_cast<size_t>(
                kUnicodeScriptIndexInvalid))
        {
            outResult.error =
                UCDScriptExtensionsParseError::StorageFailed;

            return false;
        }


        // --------------------------------------------------------------------
        // Scripts.txt must already have been parsed.
        //
        // The Script record order is intentionally the same as the alias order,
        // so an alias position is also its UnicodeScriptIndex.
        // --------------------------------------------------------------------

        if (database.scriptCount() != aliases.size())
        {
            outResult.error =
                UCDScriptExtensionsParseError::MissingScripts;

            return false;
        }


        // ====================================================================
        // Build short-name -> UnicodeScriptIndex.
        //
        // ScriptExtensions.txt uses short aliases such as:
        //
        //      Latn
        //      Arab
        //      Telu
        //
        // The alias table and Script record array have identical ordering.
        // ====================================================================

        WSOpenHashMap<InternedKey, UnicodeScriptIndex>
            scriptIndex;


        if (!scriptIndex.reserve(
            aliases.size()))
        {
            outResult.error =
                UCDScriptExtensionsParseError::StorageFailed;

            return false;
        }


        size_t aliasIndex = 0;

        for (const UCDScriptValueAlias& alias :
            aliases.aliases())
        {
            if (!alias.shortName ||
                !alias.longName)
            {
                outResult.error =
                    UCDScriptExtensionsParseError::InvalidAlias;

                return false;
            }


            if (aliasIndex >=
                static_cast<size_t>(
                    kUnicodeScriptIndexInvalid))
            {
                outResult.error =
                    UCDScriptExtensionsParseError::StorageFailed;

                return false;
            }


            const UnicodeScriptIndex index =
                static_cast<UnicodeScriptIndex>(
                    aliasIndex);


            if (!scriptIndex.put(
                alias.shortName,
                index))
            {
                outResult.error =
                    UCDScriptExtensionsParseError::StorageFailed;

                return false;
            }


            ++aliasIndex;
        }


        // ====================================================================
        // Parse explicit Script_Extensions assignments.
        // ====================================================================

        UCDParser parser(source);
        UCDLine line;

        uint32_t previousLast = 0;
        bool havePrevious = false;


        while (parser.next(line))
        {
            ByteSpan fields =
                line.data;


            // ----------------------------------------------------------------
            // Field 1: code-point range
            // ----------------------------------------------------------------

            UCDCodePointRange range;


            if (!ucdReadCodePointRange(
                fields,
                range))
            {
                outResult.error =
                    UCDScriptExtensionsParseError::InvalidRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Field 2: whitespace-separated Script short names
            // ----------------------------------------------------------------

            ByteSpan scriptList;


            if (!ucdReadField(
                fields,
                scriptList) ||
                !scriptList)
            {
                outResult.error =
                    UCDScriptExtensionsParseError::MissingScriptList;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            bspan_trim_spaces(
                scriptList);


            if (!scriptList)
            {
                outResult.error =
                    UCDScriptExtensionsParseError::MissingScriptList;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // ScriptExtensions.txt records contain exactly two fields.
            // ----------------------------------------------------------------

            bspan_trim_spaces(fields);


            if (fields)
            {
                outResult.error =
                    UCDScriptExtensionsParseError::UnexpectedField;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // ScriptExtensions.txt is expected to be globally ordered and
            // non-overlapping by code point.
            // ----------------------------------------------------------------

            if (havePrevious &&
                range.first <= previousLast)
            {
                outResult.error =
                    UCDScriptExtensionsParseError::InvalidRangeOrder;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // =================================================================
            // Build the Script set for this range.
            // =================================================================

            UnicodeScriptSet set{};

            ByteSpan scriptNameText;


            while (ucd_detail::readScriptExtensionName(
                scriptList,
                scriptNameText))
            {
                // ------------------------------------------------------------
                // Canonicalize Script short name.
                // ------------------------------------------------------------

                InternedKey scriptName =
                    WSNameSet::INTERN(
                        scriptNameText);


                if (!scriptName)
                {
                    outResult.error =
                        UCDScriptExtensionsParseError::NameInternFailed;

                    outResult.lineNumber =
                        line.lineNumber;

                    return false;
                }


                // ------------------------------------------------------------
                // Resolve short Script alias to Script record index.
                // ------------------------------------------------------------

                const UnicodeScriptIndex* indexPtr =
                    scriptIndex.getRef(
                        scriptName);


                if (!indexPtr)
                {
                    outResult.error =
                        UCDScriptExtensionsParseError::UnknownScriptName;

                    outResult.lineNumber =
                        line.lineNumber;

                    return false;
                }


                // ------------------------------------------------------------
                // Duplicate aliases in one Script_Extensions set are malformed.
                // ------------------------------------------------------------

                if (set.contains(*indexPtr))
                {
                    outResult.error =
                        UCDScriptExtensionsParseError::DuplicateScriptName;

                    outResult.lineNumber =
                        line.lineNumber;

                    return false;
                }


                set.add(
                    *indexPtr);
            }


            // ----------------------------------------------------------------
            // Every explicit Script_Extensions record must contain at least
            // one Script.
            // ----------------------------------------------------------------

            if (set.empty())
            {
                outResult.error =
                    UCDScriptExtensionsParseError::EmptyScriptSet;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // =================================================================
            // Add / deduplicate the Script set.
            // =================================================================

            UnicodeScriptSetIndex setIndex;


            if (!database.addScriptSet(
                set,
                setIndex))
            {
                outResult.error =
                    UCDScriptExtensionsParseError::ScriptSetAddFailed;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // =================================================================
            // Add the explicit code-point range.
            // =================================================================

            if (!database.addScriptExtensionRange(
                range.first,
                range.last,
                setIndex))
            {
                outResult.error =
                    UCDScriptExtensionsParseError::ScriptRangeAddFailed;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            ++outResult.rangeCount;

            outResult.explicitCodePoints +=
                static_cast<size_t>(
                    range.last -
                    range.first +
                    1u);


            previousLast =
                range.last;

            havePrevious =
                true;
        }


        // ====================================================================
        // Final statistics
        // ====================================================================

        if (database.scriptSetCount() >
            static_cast<size_t>(
                UINT32_MAX))
        {
            outResult.error =
                UCDScriptExtensionsParseError::StorageFailed;

            return false;
        }


        outResult.uniqueSetCount =
            static_cast<uint32_t>(
                database.scriptSetCount());


        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static inline bool ucdParseScriptExtensions(
        const ByteSpan& source,
        const UCDScriptValueAliases& aliases,
        UnicodeDatabaseBuilder& database)
    {
        UCDScriptExtensionsParseResult result;

        return ucdParseScriptExtensions(
            source,
            aliases,
            database,
            result);
    }

} // namespace waavs