#pragma once

#include "unicode_scalar_stream.h"

namespace waavs
{
    static bool testUtf8ScalarStream()
    {
        // UTF-8 sequence:
        //
        //      U+0041      41
        //      U+00E9      C3 A9
        //      U+20AC      E2 82 AC
        //      U+1F600     F0 9F 98 80
        //
        // Source byte ranges:
        //
        //      U+0041      [0, 1)
        //      U+00E9      [1, 3)
        //      U+20AC      [3, 6)
        //      U+1F600     [6, 10)

        static const uint8_t sourceBytes[] = {
            0x41,
            0xC3, 0xA9,
            0xE2, 0x82, 0xAC,
            0xF0, 0x9F, 0x98, 0x80
        };

        struct Expected
        {
            uint32_t value;
            TextOffset begin;
            TextOffset end;
        };

        static const Expected expected[] = {
            { 0x0041u, 0u, 1u },
            { 0x00E9u, 1u, 3u },
            { 0x20ACu, 3u, 6u },
            { 0x1F600u, 6u, 10u }
        };

        ByteSpan source(sourceBytes, sizeof(sourceBytes));
        Utf8ScalarStream stream(source);

        UnicodeScalar scalar;

        for (uint32_t i = 0; i < 4; ++i)
        {
            if (!stream(scalar))
            {
                printf("Utf8ScalarStream: FAIL - premature end at scalar %u\n", i);
                return false;
            }

            if (scalar.value != expected[i].value)
            {
                printf(
                    "Utf8ScalarStream: FAIL - scalar %u value U+%04X expected U+%04X\n",
                    i,
                    scalar.value,
                    expected[i].value);

                return false;
            }

            if (scalar.source.begin != expected[i].begin ||
                scalar.source.end != expected[i].end)
            {
                printf(
                    "Utf8ScalarStream: FAIL - scalar %u range [%u,%u) expected [%u,%u)\n",
                    i,
                    scalar.source.begin,
                    scalar.source.end,
                    expected[i].begin,
                    expected[i].end);

                return false;
            }
        }

        if (stream(scalar))
        {
            printf("Utf8ScalarStream: FAIL - produced scalar after end\n");
            return false;
        }

        if (stream.status() != TextStreamStatus::End)
        {
            printf("Utf8ScalarStream: FAIL - final status is not End\n");
            return false;
        }

        if (stream.errorOffset() != kTextOffsetInvalid)
        {
            printf("Utf8ScalarStream: FAIL - unexpected error offset\n");
            return false;
        }

        if (stream.sourceOffset() != sizeof(sourceBytes))
        {
            printf(
                "Utf8ScalarStream: FAIL - final source offset %u expected %u\n",
                stream.sourceOffset(),
                static_cast<unsigned>(sizeof(sourceBytes)));

            return false;
        }

        printf("Utf8ScalarStream: PASS\n");
        printf("  Scalars decoded: 4\n");
        printf("  Source bytes:     %u\n",
            static_cast<unsigned>(sizeof(sourceBytes)));
        printf("  Final status:     End\n");

        return true;
    }
}
