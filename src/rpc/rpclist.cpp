// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Original code was distributed under the MIT software license.
// Copyright (c) 2014-2017 Coin Sciences Ltd
// Copyright (c) 2018 Apsaras Group Ltd
// Amberchain code distributed under the GPLv3 license, see COPYING file.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif


#include "rpc/rpcserver.h"


/* MCHN START */
static const CRPCCommand vRPCWalletReadCommands[] =
{ //  category              name                      actor (function)         okSafeMode threadSafe reqWallet
  //  --------------------- ------------------------  -----------------------  ---------- ---------- ---------
    { "wallet",             "getbalance",             &getbalance,             false,     false,      true },
    { "wallet",             "getreceivedbyaccount",   &getreceivedbyaccount,   false,     false,      true },
    { "wallet",             "getreceivedbyaddress",   &getreceivedbyaddress,   false,     false,      true },
    { "wallet",             "gettransaction",         &gettransaction,         false,     false,      true },
    { "wallet",             "getunconfirmedbalance",  &getunconfirmedbalance,  false,     false,      true },
    { "wallet",             "getwalletinfo",          &getwalletinfo,          false,     false,      true },
    { "wallet",             "listlockunspent",        &listlockunspent,        false,     false,      true },
    { "wallet",             "listreceivedbyaccount",  &listreceivedbyaccount,  false,     false,      true },
    { "wallet",             "listreceivedbyaddress",  &listreceivedbyaddress,  false,     false,      true },
    { "wallet",             "listsinceblock",         &listsinceblock,         false,     false,      true },
    { "wallet",             "listtransactions",       &listtransactions,       false,     false,      true },
    { "wallet",             "listunspent",            &listunspent,            false,     false,      true },
    { "wallet",             "getassetbalances",       &getassetbalances,       false,     false,      true },
    { "wallet",             "gettotalbalances",       &gettotalbalances,       false,     false,      true },
    
    { "wallet",             "getmultibalances",       &getmultibalances,       false,     false,      true },
    { "wallet",             "getaddressbalances",     &getaddressbalances,     false,     false,      true },
    
    { "wallet",             "listwallettransactions", &listwallettransactions, false,     false,      true },
    { "wallet",             "listaddresstransactions",&listaddresstransactions,false,     false,      true },
    { "wallet",             "getwallettransaction",   &getwallettransaction,   false,     false,      true },
    { "wallet",             "getaddresstransaction",  &getaddresstransaction,  false,     false,      true },
};

/* MCHN END */

/**
 * Call Table
 */
static const CRPCCommand vRPCCommands[] =
{ //  category              name                      actor (function)         okSafeMode threadSafe reqWallet
  //  --------------------- ------------------------  -----------------------  ---------- ---------- ---------
    /* Overall control/query calls */
    { "control",            "getinfo",                &getinfo,                true,      false,      false }, /* uses wallet if enabled */
    { "control",            "help",                   &help,                   true,      true,       false },
    { "control",            "stop",                   &stop,                   true,      true,       false },
/* MCHN START */    
    { "control",            "pause",                  &pausecmd,               true,      false,      false },
    { "control",            "resume",                 &resumecmd,              true,      false,      false },
    { "control",            "clearmempool",           &clearmempool,           true,      false,      false },
    { "control",            "setlastblock",           &setlastblock,           true,      false,      false },
    { "control",            "getblockchainparams",    &getblockchainparams,    true,      false,      false }, 
    { "control",            "getruntimeparams",       &getruntimeparams,       true,      false,      false }, 
    { "control",            "setruntimeparam",        &setruntimeparam,        true,      false,      false }, 
/* MCHN END */    

    /* P2P networking */
    { "network",            "getnetworkinfo",         &getnetworkinfo,         true,      false,      false },
    { "network",            "addnode",                &addnode,                true,      true,       false },
    { "network",            "getaddednodeinfo",       &getaddednodeinfo,       true,      true,       false },
    { "network",            "getconnectioncount",     &getconnectioncount,     true,      false,      false },
    { "network",            "getnettotals",           &getnettotals,           true,      true,       false },
    { "network",            "getpeerinfo",            &getpeerinfo,            true,      false,      false },
    { "network",            "ping",                   &ping,                   true,      false,      false },

    /* Block chain and UTXO */
    { "blockchain",         "getblockchaininfo",      &getblockchaininfo,      true,      false,      false },
    { "blockchain",         "getbestblockhash",       &getbestblockhash,       true,      false,      false },
    { "blockchain",         "getblockcount",          &getblockcount,          true,      false,      false },
    { "blockchain",         "getblock",               &getblock,               true,      false,      false },
    { "blockchain",         "getblockhash",           &getblockhash,           true,      false,      false },
    { "blockchain",         "getchaintips",           &getchaintips,           true,      false,      false },
    { "blockchain",         "getdifficulty",          &getdifficulty,          true,      false,      false },
    { "blockchain",         "getmempoolinfo",         &getmempoolinfo,         true,      true,       false },
    { "blockchain",         "getrawmempool",          &getrawmempool,          true,      false,      false },
    { "blockchain",         "gettxout",               &gettxout,               true,      false,      false },
    { "blockchain",         "gettxoutsetinfo",        &gettxoutsetinfo,        true,      false,      false },
    { "blockchain",         "verifychain",            &verifychain,            true,      false,      false },
    { "blockchain",         "invalidateblock",        &invalidateblock,        true,      true,       false },
    { "blockchain",         "reconsiderblock",        &reconsiderblock,        true,      true,       false },

/* MCHN START */    
    { "blockchain",         "listassets",             &listassets,             true,      false,      false },
    { "blockchain",         "listpermissions",        &listpermissions,        true,      false,      false },
    { "blockchain",         "liststreams",            &liststreams,            true,      false,      false },
    { "blockchain",         "listupgrades",           &listupgrades,           true,      false,      false },
    { "blockchain",         "listblocks",             &listblocks,             true,      false,      false },
/* MCHN END */    
    
    /* Mining */
    { "mining",             "getblocktemplate",       &getblocktemplate,       true,      false,      false },
    { "mining",             "getmininginfo",          &getmininginfo,          true,      false,      false },
    { "mining",             "getnetworkhashps",       &getnetworkhashps,       true,      false,      false },
    { "mining",             "prioritisetransaction",  &prioritisetransaction,  true,      false,      false },
    { "mining",             "submitblock",            &submitblock,            true,      true,       false },

#ifdef ENABLE_WALLET
    /* Coin generation */
    { "generating",         "getgenerate",            &getgenerate,            true,      false,      false },
    { "generating",         "gethashespersec",        &gethashespersec,        true,      false,      false },
    { "generating",         "setgenerate",            &setgenerate,            true,      true,       false },
#endif

    /* Raw transactions */
    { "rawtransactions",    "appendrawtransaction",   &appendrawtransaction,   true,      false,      false },
    { "rawtransactions",    "createrawtransaction",   &createrawtransaction,   true,      false,      false },
    { "rawtransactions",    "decoderawtransaction",   &decoderawtransaction,   true,      false,      false },
    { "rawtransactions",    "decodescript",           &decodescript,           true,      false,      false },
    { "rawtransactions",    "getrawtransaction",      &getrawtransaction,      true,      false,      false },
    { "rawtransactions",    "sendrawtransaction",     &sendrawtransaction,     false,     false,      false },
    { "rawtransactions",    "signrawtransaction",     &signrawtransaction,     false,     false,      false }, /* uses wallet if enabled */
/* MCHN START */    
    { "rawtransactions",    "appendrawchange",        &appendrawchange,        false,     false,      true },
    { "hidden",             "appendrawmetadata",      &appendrawmetadata,      false,     false,      true },
    { "rawtransactions",    "appendrawdata",          &appendrawmetadata,      false,     false,      true },
/* MCHN END */    

    /* Utility functions */
/* AMB START */
    { "util",               "createwalletaccount",    &createwalletaccount,    true,      true ,      false },
/* AMB END */
    { "util",               "createkeypairs",         &createkeypairs,         true,      true ,      false },
    { "util",               "createmultisig",         &createmultisig,         true,      true ,      false },
    { "util",               "validateaddress",        &validateaddress,        true,      false,      false }, /* uses wallet if enabled */
    { "util",               "verifymessage",          &verifymessage,          true,      false,      false },
    { "util",               "estimatefee",            &estimatefee,            true,      true,       false },
    { "util",               "estimatepriority",       &estimatepriority,       true,      true,       false },

    /* Not shown in help */
    { "hidden",             "invalidateblock",        &invalidateblock,        true,      true,       false },
    { "hidden",             "reconsiderblock",        &reconsiderblock,        true,      true,       false },
    { "hidden",             "setmocktime",            &setmocktime,            true,      false,      false },

#ifdef ENABLE_WALLET
    /* Wallet */
    { "wallet",             "addmultisigaddress",     &addmultisigaddress,     true,      false,      true },
    /* AMB START */
    { "wallet",             "getauthmultisigaddress",   &getauthmultisigaddress,    true,      false,      true },
    { "wallet",             "getescrowmultisigaddress", &getescrowmultisigaddress,  true,      false,      true },
    /* AMB END */
    { "wallet",             "backupwallet",           &backupwallet,           true,      false,      true },
    { "wallet",             "dumpprivkey",            &dumpprivkey,            true,      false,      true },
    { "wallet",             "dumpwallet",             &dumpwallet,             true,      false,      true },
    { "wallet",             "encryptwallet",          &encryptwallet,          true,      false,      true },
    { "wallet",             "getaccountaddress",      &getaccountaddress,      true,      false,      true },
    { "wallet",             "getaccount",             &getaccount,             true,      false,      true },
    { "wallet",             "getaddressesbyaccount",  &getaddressesbyaccount,  true,      false,      true },
    { "wallet",             "getbalance",             &getbalance,             false,     false,      true },
    { "wallet",             "getnewaddress",          &getnewaddress,          true,      false,      true },
    { "wallet",             "getrawchangeaddress",    &getrawchangeaddress,    true,      false,      true },
    { "wallet",             "getreceivedbyaccount",   &getreceivedbyaccount,   false,     false,      true },
    { "wallet",             "getreceivedbyaddress",   &getreceivedbyaddress,   false,     false,      true },
    { "wallet",             "gettransaction",         &gettransaction,         false,     false,      true },
    { "wallet",             "getunconfirmedbalance",  &getunconfirmedbalance,  false,     false,      true },
    { "wallet",             "getwalletinfo",          &getwalletinfo,          false,     false,      true },
    { "wallet",             "importprivkey",          &importprivkey,          true,      false,      true },
    { "wallet",             "importwallet",           &importwallet,           true,      false,      true },
    { "wallet",             "importaddress",          &importaddress,          true,      false,      true },
    { "wallet",             "keypoolrefill",          &keypoolrefill,          true,      false,      true },
    { "wallet",             "listaccounts",           &listaccounts,           false,     false,      true },
    { "wallet",             "listaddressgroupings",   &listaddressgroupings,   false,     false,      true },
    { "wallet",             "listlockunspent",        &listlockunspent,        false,     false,      true },
    { "wallet",             "listreceivedbyaccount",  &listreceivedbyaccount,  false,     false,      true },
    { "wallet",             "listreceivedbyaddress",  &listreceivedbyaddress,  false,     false,      true },
    { "wallet",             "listsinceblock",         &listsinceblock,         false,     false,      true },
    { "wallet",             "listtransactions",       &listtransactions,       false,     false,      true },
    { "wallet",             "listunspent",            &listunspent,            false,     false,      true },
    { "wallet",             "lockunspent",            &lockunspent,            true,      false,      true },
    { "wallet",             "move",                   &movecmd,                false,     false,      true },
//    { "wallet",             "sendfrom",               &sendfrom,               false,     false,      true },
    { "wallet",             "sendfromaccount",        &sendfrom,               false,     false,      true },
    { "wallet",             "sendmany",               &sendmany,               false,     false,      true },
    { "hidden",             "sendtoaddress",          &sendtoaddress,          false,     false,      true },
    { "wallet",             "send",                   &sendtoaddress,          false,     false,      true },
/* AMB START */
    { "wallet",             "registeraddress",        &registeraddress,        false,     false,      true }, 
    { "wallet",             "getpubkeyforaddress",    &getpubkeyforaddress,    false,     false,      true }, 
    { "wallet",             "approveauthority",       &approveauthority,       false,     false,      true }, 
    { "wallet",             "requestauthority",       &requestauthority,       false,     false,      true }, 
    { "wallet",             "annotaterecord",         &annotaterecord,         false,     false,      true }, 
    { "wallet",             "revokerecord",           &revokerecord,           false,     false,      true }, 
    { "wallet",             "writerecord",            &writerecord,            false,     false,      true },
    { "wallet",             "createbulletinboard",    &createbulletinboard,    false,     false,      true }, 
    { "wallet",             "writebulletinboardentry",  &writebulletinboardentry,   false,     false,      true },
    { "wallet",             "censormessage",          &censormessage,   false,     false,      true },     
    { "wallet",             "grantmessagecensorpermission", &grantmessagecensorpermission,   false,     false,      true },     
    { "wallet",             "revokemessagecensorpermission", &revokemessagecensorpermission,   false,     false,      true },     
    { "wallet",             "annotatebadge",          &annotatebadge,          false,     false,      true },
    { "wallet",             "createbadge",            &createbadge,            false,     false,      true },
    { "wallet",             "updatebadge",            &updatebadge,            false,     false,      true },
    { "wallet",             "issuebadge",             &issuebadge,             false,     false,      true },
    { "wallet",             "revokebadge",            &revokebadge,            false,     false,      true },
    { "wallet",             "requestissuebadge",      &requestissuebadge,      false,     false,      true },
    { "wallet",             "processrequestissuebadge",     &processrequestissuebadge,      false,  false,  true },
    { "wallet",             "grantbadgeissuerpermission",   &grantbadgeissuerpermission,    false,  false,  true },
    { "wallet",             "revokebadgeissuerpermission",  &revokebadgeissuerpermission,   false,  false,  true },
    { "wallet",             "writecategory",          &writecategory,          false,     false,      true },
    { "wallet",             "updatecategory",         &updatecategory,         false,     false,      true },
    { "wallet",             "deletecategory",         &deletecategory,         false,     false,      true },
    { "wallet",             "writecustomcategory",    &writecustomcategory,    false,     false,      true },
    { "wallet",             "updatecustomcategory",   &updatecustomcategory,   false,     false,      true },
    { "wallet",             "deletecustomcategory",   &deletecustomcategory,   false,     false,      true },
    { "wallet",             "writerecordtype",        &writerecordtype,        false,     false,      true },
    { "wallet",             "listservice",            &listservice,            false,     false,      true },
    { "wallet",             "updateservice",          &updateservice,          false,     false,      true },
    { "wallet",             "delistservice",          &delistservice,          false,     false,      true },
    { "wallet",             "purchasenonconsumableservice", &purchasenonconsumableservice,  false,  false,  true },
    { "wallet",             "purchaseconsumableservice",    &purchaseconsumableservice,     false,  false,  true },
    { "wallet",             "delistservice",          &delistservice,          false,     false,      true },
    { "wallet",             "logactivity",            &logactivity,            false,     false,      true },
    { "wallet",             "sharetxn",               &sharetxn,               false,     false,      true },
    { "wallet",             "addservicequantity",     &addservicequantity,     false,     false,      true },
    { "wallet",             "removeservicequantity",  &removeservicequantity,  false,     false,      true },
    { "wallet",             "updatepurchasestatus",   &updatepurchasestatus,   false,     false,      true },
    { "wallet",             "completepurchase",       &completepurchase,       false,     false,      true },
    { "wallet",             "refundpurchase",         &refundpurchase,         false,     false,      true },
    { "wallet",             "expirepurchase",         &expirepurchase,         false,     false,      true },
    { "wallet",             "getservice",             &getservice,             false,     false,      true },
    { "wallet",             "appendrawsendfrom",      &appendrawsendfrom,      false,     false,      true },
    { "wallet",             "verifyblocksignature",   &verifyblocksignature,   false,     false,      true },
    
/* AMB END */
/* MCHN START */    
    { "wallet",             "getaddresses",           &getaddresses,           true,      false,      true },
    { "wallet",             "combineunspent",         &combineunspent,         false,     false,      true }, 
    { "wallet",             "grant",                  &grantcmd,               false,     false,      true }, 
    { "wallet",             "revoke",                 &revokecmd,              false,     false,      true },
    { "wallet",             "issue",                  &issuecmd,               false,     false,      true },
    { "wallet",             "issuemore",              &issuemorecmd,           false,     false,      true },
    { "wallet",             "getassetbalances",       &getassetbalances,       false,     false,      true },
    { "wallet",             "gettotalbalances",       &gettotalbalances,       false,     false,      true },
    { "hidden",             "sendassettoaddress",     &sendassettoaddress,     false,     false,      true },
    { "wallet",             "sendasset",              &sendassettoaddress,     false,     false,      true },
    { "wallet",             "preparelockunspent",     &preparelockunspent,     false,     false,      true },
    { "wallet",             "createrawexchange",      &createrawexchange,      false,     false,      true },
    { "wallet",             "appendrawexchange",      &appendrawexchange,      false,     false,      true },
    { "wallet",             "completerawexchange",    &completerawexchange,    false,     false,      true },
    { "wallet",             "decoderawexchange",      &decoderawexchange,      false,     false,      true },
    
    { "wallet",             "grantfrom",              &grantfromcmd,           false,     false,      true }, 
    { "wallet",             "approvefrom",            &approvefrom,            false,     false,      true }, 
    { "wallet",             "revokefrom",             &revokefromcmd,          false,     false,      true },
    { "wallet",             "issuefrom",              &issuefromcmd,           false,     false,      true },
    { "wallet",             "issuemorefrom",          &issuemorefromcmd,       false,     false,      true },
    { "wallet",             "preparelockunspentfrom", &preparelockunspentfrom, false,     false,      true },
    { "wallet",             "sendassetfrom",          &sendassetfrom,          false,     false,      true },
    { "hidden",             "sendfromaddress",        &sendfromaddress,        false,     false,      true },
    { "wallet",             "sendfrom",               &sendfromaddress,        false,     false,      true },
    { "wallet",             "getmultibalances",       &getmultibalances,       false,     false,      true },
    { "wallet",             "getaddressbalances",     &getaddressbalances,     false,     false,      true },
    { "wallet",             "disablerawtransaction",  &disablerawtransaction,  false,     false,      true },
    { "hidden",             "sendwithmetadata",       &sendwithmetadata,       false,     false,      true },
    { "wallet",             "sendwithdata",           &sendwithmetadata,       false,     false,      true },
    { "hidden",             "sendwithmetadatafrom",   &sendwithmetadatafrom,   false,     false,      true },
    { "wallet",             "sendwithdatafrom",       &sendwithmetadatafrom,   false,     false,      true },
    { "hidden",             "grantwithmetadata",      &grantwithmetadata,      false,     false,      true },
    { "wallet",             "grantwithdata",          &grantwithmetadata,      false,     false,      true },
    { "hidden",             "grantwithmetadatafrom",  &grantwithmetadatafrom,  false,     false,      true },
    { "wallet",             "grantwithdatafrom",      &grantwithmetadatafrom,  false,     false,      true },
    { "wallet",             "createrawsendfrom",      &createrawsendfrom,      false,     false,      true },
    
    { "wallet",             "listaddresses",          &listaddresses,          false,     false,      true },
    { "wallet",             "listwallettransactions", &listwallettransactions, false,     false,      true },
    { "wallet",             "listaddresstransactions",&listaddresstransactions,false,     false,      true },
    { "wallet",             "getwallettransaction",   &getwallettransaction,   false,     false,      true },
    { "wallet",             "getaddresstransaction",  &getaddresstransaction,  false,     false,      true },
    { "wallet",             "resendwallettransactions",&resendwallettransactions,  false,     false,      true },
    
    { "wallet",             "create",                 &createcmd,                false,     false,      true },
    { "wallet",             "createfrom",             &createfromcmd,            false,     false,      true },
    { "wallet",             "publish",                &publish,                false,     false,      true },
    { "wallet",             "publishfrom",            &publishfrom,            false,     false,      true },
    { "wallet",             "subscribe",              &subscribe,              false,     false,      true },
    { "wallet",             "unsubscribe",            &unsubscribe,            false,     false,      true },
    { "wallet",             "listassettransactions",  &listassettransactions,         false,     false,      true },
    { "wallet",             "getassettransaction",    &getassettransaction,         false,     false,      true },
    { "wallet",             "getstreamitem",          &getstreamitem,         false,     false,      true },
    { "wallet",             "liststreamitems",        &liststreamitems,         false,     false,      true },
    { "wallet",             "liststreamkeyitems",     &liststreamkeyitems,      false,     false,      true },
    { "wallet",             "liststreampublisheritems",&liststreampublisheritems,false,     false,      true },
    { "wallet",             "liststreamkeys",         &liststreamkeys,        false,     false,      true },
    { "wallet",             "liststreampublishers",   &liststreampublishers,  false,     false,      true },
    { "wallet",             "gettxoutdata",           &gettxoutdata,           false,     false,      true },
    { "wallet",             "liststreamblockitems",   &liststreamblockitems,    false,      false,      false },
    
/* MCHN END */    
    { "wallet",             "setaccount",             &setaccount,             true,      false,      true },
    { "wallet",             "settxfee",               &settxfee,               true,      false,      true },
    { "wallet",             "signmessage",            &signmessage,            true,      false,      true },
    { "wallet",             "walletlock",             &walletlock,             true,      false,      true },
    { "wallet",             "walletpassphrasechange", &walletpassphrasechange, true,      false,      true },
    { "wallet",             "walletpassphrase",       &walletpassphrase,       true,      false,      true },
#endif // ENABLE_WALLET
};

void mc_InitRPCList(std::vector<CRPCCommand>& vStaticRPCCommands,std::vector<CRPCCommand>& vStaticRPCWalletReadCommands)
{
    unsigned int vcidx;
    vStaticRPCCommands.clear();
    vStaticRPCWalletReadCommands.clear();
    for (vcidx = 0; vcidx < (sizeof(vRPCCommands) / sizeof(vRPCCommands[0])); vcidx++)
    {
        vStaticRPCCommands.push_back(vRPCCommands[vcidx]);
    }    
    for (vcidx = 0; vcidx < (sizeof(vRPCWalletReadCommands) / sizeof(vRPCWalletReadCommands[0])); vcidx++)
    {
        vStaticRPCWalletReadCommands.push_back(vRPCCommands[vcidx]);
    }    
}
