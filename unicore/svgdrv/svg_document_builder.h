// svg_document_builder.h
#pragma once

#include <charconv>
#include <cmath>
#include <string>

namespace waavs {

    class SVGDocumentBuilder
    {
    public:
        std::string defs;
        std::string body;

        bool addPathDefinition(const std::string& id, const std::string& d)
        {
            defs += "    <path id=\"";
            appendEscaped(defs, id);
            defs += "\" d=\"";
            appendEscaped(defs, d);
            defs += "\"/>\n";
            return true;
        }

        bool addUse(const std::string& id, float tx, float ty, float sx, float sy)
        {
            body += "  <use href=\"#";
            appendEscaped(body, id);
            body += "\" transform=\"translate(";

            if (!appendNumber(body, tx))
                return false;

            body.push_back(' ');

            if (!appendNumber(body, ty))
                return false;

            body += ") scale(";

            if (!appendNumber(body, sx))
                return false;

            body.push_back(' ');

            if (!appendNumber(body, sy))
                return false;

            body += ")\"/>\n";
            return true;
        }

        bool addGlyphUse(const std::string& id, float x, float y, float scale)
        {
            return addUse(id, x, y, scale, -scale);
        }

        std::string document(float minX, float minY, float width, float height) const
        {
            std::string out;

            out += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            out += "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"";

            appendNumber(out, minX);
            out.push_back(' ');
            appendNumber(out, minY);
            out.push_back(' ');
            appendNumber(out, width);
            out.push_back(' ');
            appendNumber(out, height);

            out += "\">\n";

            if (!defs.empty()) {
                out += "  <defs>\n";
                out += defs;
                out += "  </defs>\n";
            }

            out += body;
            out += "</svg>\n";

            return out;
        }

        void clear()
        {
            defs.clear();
            body.clear();
        }

    private:
        static bool appendNumber(std::string& dst, float value)
        {
            if (!std::isfinite(value))
                return false;

            if (value == 0.0f)
                value = 0.0f;

            char buffer[64];
            auto result = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general);

            if (result.ec != std::errc())
                return false;

            dst.append(buffer, result.ptr);
            return true;
        }

        static void appendEscaped(std::string& dst, const std::string& src)
        {
            for (char c : src) {
                switch (c) {
                case '&': dst += "&amp;"; break;
                case '<': dst += "&lt;"; break;
                case '>': dst += "&gt;"; break;
                case '"': dst += "&quot;"; break;
                case '\'': dst += "&apos;"; break;
                default: dst.push_back(c); break;
                }
            }
        }
    };

}