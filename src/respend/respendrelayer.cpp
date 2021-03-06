// Copyright (c) 2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "respend/respendrelayer.h"
#include "net.h" // RelayTransaction
#include "options.h"
#include "protocol.h"
#include "streams.h"
#include "util.h"
#include "utilprocessmsg.h"
#include "policy/policy.h"
#include <mutex>

namespace respend {

RespendRelayer::RespendRelayer(CConnman* connman) :
    interesting(false), valid(false), connman(connman)
{
    if (!connman)
        throw std::invalid_argument(std::string(__func__ ) + " requires connection manager");
}

bool RespendRelayer::AddOutpointConflict(
        const COutPoint&, const CTxMemPool::txiter mempoolEntry,
        const CTransaction& respendTx, bool seenBefore,
        bool isEquivalent, bool isSSCandidate)
{
    if (!isSSCandidate)
        return false;

    if (seenBefore || isEquivalent || !mempoolEntry->IsLiveSS())
        return true; // look at more outpoints

    // Can the respend be SS? Slightly more expensive checks
    if (!IsSuperStandardTx(respendTx, isSSCandidate))
        return false;

    respend = respendTx;
    interesting = true;
    return false;
}

bool RespendRelayer::IsInteresting() const {
    return interesting;
}

void RespendRelayer::OnValidTrigger(bool v, CTxMemPool&,
        CTxMemPool::setEntries&) {
    valid = v;
}

void RespendRelayer::OnFinishedTrigger() {
    if (!valid || !interesting)
        return;

    std::vector<uint256> vAncestors;
    vAncestors.push_back(respend.GetHash()); // Alert only for the tx itself
    connman->RelayTransaction(respend, vAncestors, true);
}

} // ns respend
