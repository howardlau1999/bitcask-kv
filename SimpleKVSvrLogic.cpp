#include "SimpleKVSvr.h"
std::string SimpleKVSvr::get(std::vector<std::string> const& args) {
    std::string strReply;
    if (args.size() < 2) return strReply;
    if (m_oCache->get(args[1], strReply)) {
        SimpleKVRecord oRecord;
        int iRet = m_oIndex.get(args[1], oRecord);
        if (iRet == 0) {
            strReply = m_oStore.getValue(oRecord);
            m_oCache->set(args[1], strReply);
            ++m_cntHit;
        } else {
            ++m_cntMiss;
        }
    } else {
        ++m_cntHit;
    }
    return strReply;
}

std::string SimpleKVSvr::remove(std::vector<std::string> const& args) {
    if (args.size() < 2) return "";
    SimpleKVRecord stRecord = m_oStore.remove(args[1]);
    m_oIndex.remove(stRecord);
    m_oCache->invalidate(args[1]);
    return "OK";
}

std::string SimpleKVSvr::stats(std::vector<std::string> const&) {
    return statsString();
}

std::string SimpleKVSvr::set(std::vector<std::string> const& args) {
    if (args.size() < 3) return "";
    SimpleKVRecord stRecord = m_oStore.append(args[1], args[2]);
    m_oCache->invalidate(args[1]);
    m_oIndex.set(stRecord);
    return "OK";
}

std::string SimpleKVSvr::compact(std::vector<std::string> const&) {
    m_oStore.compact();
    return "OK";
}