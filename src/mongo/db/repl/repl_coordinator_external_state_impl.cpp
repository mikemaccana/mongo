/**
 *    Copyright (C) 2014 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */
 
#include "mongo/platform/basic.h"

#include "mongo/platform/basic.h"

#include "mongo/db/repl/repl_coordinator_external_state_impl.h"

#include <string>

#include "mongo/bson/oid.h"
#include "mongo/db/client.h"
#include "mongo/db/dbhelpers.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/operation_context_impl.h"
#include "mongo/db/repl/isself.h"
#include "mongo/util/net/hostandport.h"
#include "mongo/util/net/sock.h"

namespace mongo {
namespace repl {

    ReplicationCoordinatorExternalStateImpl::ReplicationCoordinatorExternalStateImpl() {}
    ReplicationCoordinatorExternalStateImpl::~ReplicationCoordinatorExternalStateImpl() {}

    void ReplicationCoordinatorExternalStateImpl::runSyncSourceFeedback() {
        _syncSourceFeedback.run();
    }

    void ReplicationCoordinatorExternalStateImpl::shutdown() {
        _syncSourceFeedback.shutdown();
    }

    void ReplicationCoordinatorExternalStateImpl::forwardSlaveHandshake() {
        _syncSourceFeedback.forwardSlaveHandshake();
    }

    void ReplicationCoordinatorExternalStateImpl::forwardSlaveProgress() {
        _syncSourceFeedback.forwardSlaveProgress();
    }

    OID ReplicationCoordinatorExternalStateImpl::ensureMe() {
        std::string myname = getHostName();
        OID myRID;
        {
            OperationContextImpl txn;
            Client::WriteContext ctx(&txn, "local");

            BSONObj me;
            // local.me is an identifier for a server for getLastError w:2+
            if (!Helpers::getSingleton(&txn, "local.me", me) ||
                    !me.hasField("host") ||
                    me["host"].String() != myname) {

                myRID = OID::gen();

                // clean out local.me
                Helpers::emptyCollection(&txn, "local.me");

                // repopulate
                BSONObjBuilder b;
                b.append("_id", myRID);
                b.append("host", myname);
                Helpers::putSingleton(&txn, "local.me", b.done());
            } else {
                myRID = me["_id"].OID();
            }
            ctx.commit();
        }
        return myRID;
    }

    bool ReplicationCoordinatorExternalStateImpl::isSelf(const HostAndPort& host) {
        return repl::isSelf(host);

    }

    HostAndPort ReplicationCoordinatorExternalStateImpl::getClientHostAndPort(
            const OperationContext* txn) {
        return HostAndPort(txn->getClient()->clientAddress(true));
    }

} // namespace repl
} // namespace mongo
