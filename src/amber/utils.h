/* AMB START */

// Amberchain code distributed under the GPLv3 license, see COPYING file.

#ifndef AMBER_CONSTANTS_H
#define AMBER_CONSTANTS_H

#include <string>
#include <map>
#include <vector>

#define STREAM_AUTHNODES                    "authoritynodes"
#define STREAM_AUTHREQUESTS                 "authorityrequests"
#define STREAM_TRANSACTIONPARAMS            "transactionparams"
#define STREAM_RECORDS                      "records"
#define STREAM_BULLETINBOARDS               "bulletinboards"
#define STREAM_BULLETINBOARDCREATORS        "bulletinboardcreators"
#define STREAM_CENSOREDMESSAGES             "censoredmessages"
#define STREAM_POSTINGRIGHTS                "postingrights"
#define STREAM_CENSORINGRIGHTS              "censoringrights"
#define STREAM_BADGES                       "badges"
#define STREAM_ISSUEDBADGES                 "issuedbadges"
#define STREAM_ISSUEBADGEREQUESTS           "issuebadgerequests"
#define STREAM_ANNOTATEDBADGES              "annotatedbadges"
#define STREAM_BADGEISSUERS                 "badgeissuers"
#define STREAM_CATEGORIES                   "categories"
#define STREAM_UPDATECATEGORIES             "updatecategories"
#define STREAM_CUSTOMCATEGORIES             "customcategories"
#define STREAM_UPDATECUSTOMCATEGORIES       "updatecustomcategories"
#define STREAM_RECORDTYPES                  "recordtypes"
#define STREAM_SERVICES                     "services"
#define STREAM_INVALIDBLOCKS                "invalidblocks"
#define STREAM_ADDRESSKEYS                  "addresskeys"
#define STREAM_ACTIVITIES                   "activities"
#define STREAM_SHAREDTXNS                   "sharedtxns"
#define STREAM_PROCESSISSUEBADGEREQUESTS    "processissuebadgerequests"
#define STREAM_PURCHASESTATUS               "purchasestatus"
#define STREAM_MULTISIGS                    "multisigs"
#define KEY_TRANSACTIONFEE                  "min-relay-tx-fee"
#define KEY_ADMINPUBLICKEY                  "admin-public-key"
#define KEY_ADMINFEERATIO                   "admin-fee-ratio"
#define KEY_PUBLICACCOUNT                   "public-account"

using namespace std;

/*

To get the streams relevant to a permission:
    vector<std::string> streams = StreamConsts::streamsPerPermission.at("admin");
    for(std::vector<std::string>::iterator it = streams.begin(); it != streams.end(); ++it) {
        std::string stream_name = *it;
        throw runtime_error(stream_name);
    }

 */
struct StreamConsts 
{
    static map< string, vector<string> > create_map()
    {
        map< string, vector<string> > m;

		vector<string> admin_streams;
		admin_streams.push_back(STREAM_AUTHNODES);
        admin_streams.push_back(STREAM_TRANSACTIONPARAMS);
        admin_streams.push_back(STREAM_CATEGORIES);
        admin_streams.push_back(STREAM_UPDATECATEGORIES);
        admin_streams.push_back(STREAM_RECORDTYPES);
        // admin_streams.push_back(STREAM_INVALIDBLOCKS);
		m["admin"] = admin_streams;

		vector<string> mine_streams;
        mine_streams.push_back(STREAM_SERVICES);
        mine_streams.push_back(STREAM_BADGES);
        mine_streams.push_back(STREAM_BADGEISSUERS);
        mine_streams.push_back(STREAM_ANNOTATEDBADGES);
        mine_streams.push_back(STREAM_ISSUEDBADGES);
        mine_streams.push_back(STREAM_PROCESSISSUEBADGEREQUESTS);
		m["mine"] = mine_streams;
        return m;
    }
    static const map< string, vector<string> > streamsPerPermission;
};

string SampleFunction();

#endif

/* AMB END */