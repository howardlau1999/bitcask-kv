#include "Protocol.h"

std::vector<std::string> Protocol::splitSpace(std::string const& aString) {
    std::vector<std::string> arrPieces;
    std::string strPiece;
    for (auto const ch : aString) {
        if (ch != ' ') {
            strPiece += ch;
        } else if (!strPiece.empty()) {
            arrPieces.push_back(strPiece);
            strPiece.clear();
        }
    }
    if (!strPiece.empty()) {
        arrPieces.push_back(strPiece);
        strPiece.clear();
    }
    return arrPieces;
}

bool Protocol::parseRequestLine(std::string const& aRequestLine, std::vector<std::string>& outResult) {
    if (aRequestLine.empty() || aRequestLine[0] != '-') {
        return false;
    }

    outResult = Protocol::splitSpace(aRequestLine.substr(1));
    if (outResult.empty() || outResult.size() > 3) {
        return false;
    }

    return true;
}