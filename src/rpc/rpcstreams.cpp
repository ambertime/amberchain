// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2014-2016 The Bitcoin Core developers
// Original code was distributed under the MIT software license.
// Copyright (c) 2014-2017 Coin Sciences Ltd
// Copyright (c) 2018 Apsaras Group Ltd
// Amberchain code distributed under the GPLv3 license, see COPYING file.


#include "rpc/rpcwallet.h"
#include "amber/utils.h"
#include "amber/strencodings.h"
#include "amber/streamutils.h"
#include <sstream>
#include "utils/util.h"

Value createupgradefromcmd(const Array& params, bool fHelp);

void parseStreamIdentifier(Value stream_identifier,mc_EntityDetails *entity)
{
    unsigned char buf[32];
    unsigned char buf_a[MC_AST_ASSET_REF_SIZE];
    unsigned char buf_n[MC_AST_ASSET_REF_SIZE];
    int ret;
    
    if (stream_identifier.type() != null_type && !stream_identifier.get_str().empty())
    {        
        string str=stream_identifier.get_str();
        
        if(AssetRefDecode(buf_a,str.c_str(),str.size()))
        {
            memset(buf_n,0,MC_AST_ASSET_REF_SIZE);
            if(memcmp(buf_a,buf_n,4) == 0)
            {
                unsigned char *root_stream_name;
                int root_stream_name_size;
                root_stream_name=(unsigned char *)mc_gState->m_NetworkParams->GetParam("rootstreamname",&root_stream_name_size);        
                if(mc_gState->m_NetworkParams->IsProtocolMultichain() == 0)
                {
                    root_stream_name_size=0;
                }    
                if( (root_stream_name_size > 1) && (memcmp(buf_a,buf_n,MC_AST_ASSET_REF_SIZE) == 0) )
                {
                    str=strprintf("%s",root_stream_name);
                }
                else
                {
                    throw JSONRPCError(RPC_ENTITY_NOT_FOUND, "Stream with this stream reference not found: "+str);                    
                }
            }
        }
        
        ret=ParseAssetKey(str.c_str(),buf,NULL,NULL,NULL,NULL,MC_ENT_TYPE_STREAM);
        switch(ret)
        {
            case MC_ASSET_KEY_INVALID_TXID:
                throw JSONRPCError(RPC_ENTITY_NOT_FOUND, "Stream with this txid not found: "+str);
                break;
            case MC_ASSET_KEY_INVALID_REF:
                throw JSONRPCError(RPC_ENTITY_NOT_FOUND, "Stream with this stream reference not found: "+str);
                break;
            case MC_ASSET_KEY_INVALID_NAME:
                throw JSONRPCError(RPC_ENTITY_NOT_FOUND, "Stream with this name not found: "+str);
                break;
            case MC_ASSET_KEY_INVALID_SIZE:
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Could not parse stream key: "+str);
                break;
/*                
            case 1:
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Unconfirmed stream: "+str);
                break;
 */ 
        }
    }
    else
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid stream identifier");        
    }
           
    if(entity)
    {
        if(mc_gState->m_Assets->FindEntityByTxID(entity,buf))
        {
            if(entity->GetEntityType() != MC_ENT_TYPE_STREAM)
            {
                throw JSONRPCError(RPC_ENTITY_NOT_FOUND, "Invalid stream identifier, not stream");                        
            }
        }    
    }    
}

Value liststreams(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 4)
        throw runtime_error("Help message not found\n");

    Array results;
    mc_Buffer *streams;
    unsigned char *txid;
    uint32_t output_level;
    
    int count,start;
    count=2147483647;
    if (params.size() > 2)    
    {
        count=paramtoint(params[2],true,0,"Invalid count");
    }
    start=-count;
    if (params.size() > 3)    
    {
        start=paramtoint(params[3],false,0,"Invalid start");
    }
    
    streams=NULL;
    
    vector<string> inputStrings;
    if (params.size() > 0 && params[0].type() != null_type && ((params[0].type() != str_type) || (params[0].get_str() !="*" ) ) )
    {        
        if(params[0].type() == str_type)
        {
            inputStrings.push_back(params[0].get_str());
            if(params[0].get_str() == "")
            {
                return results;                
            }
        }
        else
        {
            inputStrings=ParseStringList(params[0]);        
            if(inputStrings.size() == 0)
            {
                return results;
            }
        }
    }
    if(inputStrings.size())
    {
        {
            LOCK(cs_main);
            for(int is=0;is<(int)inputStrings.size();is++)
            {
                string param=inputStrings[is];

                mc_EntityDetails stream_entity;
                parseStreamIdentifier(param,&stream_entity);           

                streams=mc_gState->m_Assets->GetEntityList(streams,stream_entity.GetTxID(),MC_ENT_TYPE_STREAM);
            }
        }
    }
    else
    {        
        {
            LOCK(cs_main);
            streams=mc_gState->m_Assets->GetEntityList(streams,NULL,MC_ENT_TYPE_STREAM);
        }
    }
    
    if(streams == NULL)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Cannot open entity database");

    output_level=0x1E;
    
    if (params.size() > 1)    
    {
        if(paramtobool(params[1]))
        {
            output_level=0x3E;            
        }
    }
    
    
    int root_stream_name_size;
    mc_gState->m_NetworkParams->GetParam("rootstreamname",&root_stream_name_size);        
    if( (root_stream_name_size <= 1) && (inputStrings.size() == 0) && (mc_gState->m_Features->FixedIn10008() == 0) )            // Patch, to be removed in 10008
    {
        mc_AdjustStartAndCount(&count,&start,streams->GetCount()-1);        
        start++;            
    }
    else
    {
        mc_AdjustStartAndCount(&count,&start,streams->GetCount());        
    }
    
    
    Array partial_results;
    int unconfirmed_count=0;
    if(count > 0)
    {
        for(int i=0;i<streams->GetCount();i++)
        {
            Object entry;

            txid=streams->GetRow(i);
            entry=StreamEntry(txid,output_level);
            if(entry.size()>0)
            {
                BOOST_FOREACH(const Pair& p, entry) 
                {
                    if(p.name_ == "streamref")
                    {
                        if(p.value_.type() == str_type)
                        {
                            results.push_back(entry);                        
                        }
                        else
                        {
                            unconfirmed_count++;
                        }
                    }
                }            
            }            
        }

        sort(results.begin(), results.end(), AssetCompareByRef);
        
        for(int i=0;i<streams->GetCount();i++)
        {
            Object entry;

            txid=streams->GetRow(i);

            entry=StreamEntry(txid,output_level);
            if(entry.size()>0)
            {
                BOOST_FOREACH(const Pair& p, entry) 
                {
                    if(p.name_ == "streamref")
                    {
                        if(p.value_.type() != str_type)
                        {
                            results.push_back(entry);                        
                        }
                    }
                }            
            }            
        }
    }
        
    bool return_partial=false;
    if(count != streams->GetCount()-1)
    {
        return_partial=true;
    }
    if( (root_stream_name_size <= 1) && (inputStrings.size() == 0)  && (mc_gState->m_Features->FixedIn10008() == 0) )            // Patch, to be removed in 10008
    {
        return_partial=true;        
    }
    mc_gState->m_Assets->FreeEntityList(streams);
    if(return_partial)
    {
        for(int i=start;i<start+count;i++)
        {
            partial_results.push_back(results[i]);                                                                
        }
        return partial_results;
    }
     
    return results;
}

Value createstreamfromcmd(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 4)
        throw runtime_error("Help message not found\n");

    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
    
    if (strcmp(params[1].get_str().c_str(),"stream"))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid entity type, should be stream");

    CWalletTx wtx;
    
    mc_Script *lpScript=mc_gState->m_TmpBuffers->m_RpcScript3;   
    lpScript->Clear();
    mc_Script *lpDetailsScript=mc_gState->m_TmpBuffers->m_RpcScript1;    
    lpDetailsScript->Clear();
    mc_Script *lpDetails=mc_gState->m_TmpBuffers->m_RpcScript2;
    lpDetails->Clear();
    
    int ret,type;
    string stream_name="";

    if (params[2].type() != str_type)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid stream name, should be string");
            
    if(!params[2].get_str().empty())
    {        
        stream_name=params[2].get_str();
    }
    
    if(params[3].type() != bool_type)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid open flag, should be boolean");
    
    if(mc_gState->m_Features->Streams())
    {
        if(stream_name == "*")
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid stream name: *");                                                                                            
        }
    }

    unsigned char buf_a[MC_AST_ASSET_REF_SIZE];    
    if(AssetRefDecode(buf_a,stream_name.c_str(),stream_name.size()))
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid stream name, looks like a stream reference");                                                                                                    
    }
            
    
    if(stream_name.size())
    {
        ret=ParseAssetKey(stream_name.c_str(),NULL,NULL,NULL,NULL,&type,MC_ENT_TYPE_ANY);
        if(ret != MC_ASSET_KEY_INVALID_NAME)
        {
            if(type == MC_ENT_KEYTYPE_NAME)
            {
                throw JSONRPCError(RPC_DUPLICATE_NAME, "Stream or asset with this name already exists");                                    
            }
            else
            {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid stream name");                                    
            }
        }        
    }

    lpScript->Clear();
    
    lpDetails->Clear();
    lpDetails->AddElement();
    if(params[3].get_bool())
    {
        unsigned char b=1;        
        lpDetails->SetSpecialParamValue(MC_ENT_SPRM_ANYONE_CAN_WRITE,&b,1);        
    }
    if(stream_name.size())
    {        
        lpDetails->SetSpecialParamValue(MC_ENT_SPRM_NAME,(unsigned char*)(stream_name.c_str()),stream_name.size());//+1);
    }
    
    

    if (params.size() > 4)
    {
        if(params[4].type() == obj_type)
        {
            Object objParams = params[4].get_obj();
            BOOST_FOREACH(const Pair& s, objParams) 
            {  
                lpDetails->SetParamValue(s.name_.c_str(),s.name_.size(),(unsigned char*)s.value_.get_str().c_str(),s.value_.get_str().size());                
            }
        }
        else
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid custom fields, expecting object");                                        
        }
    }
    
    int err;
    size_t bytes;
    const unsigned char *script;
    script=lpDetails->GetData(0,&bytes);
    
    size_t elem_size;
    const unsigned char *elem;
    CScript scriptOpReturn=CScript();
    
    if(mc_gState->m_Features->OpDropDetailsScripts())
    {
        err=lpDetailsScript->SetNewEntityType(MC_ENT_TYPE_STREAM,0,script,bytes);
        if(err)
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid custom fields or stream name, too long");                                                        
        }
        
        elem = lpDetailsScript->GetData(0,&elem_size);
        scriptOpReturn << vector<unsigned char>(elem, elem + elem_size) << OP_DROP << OP_RETURN;        
    }
    else
    {
        lpDetailsScript->SetNewEntityType(MC_ENT_TYPE_STREAM);

        err=lpDetailsScript->SetGeneralDetails(script,bytes);
        if(err)
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid custom fields or stream name, too long");                                                    
        }

        for(int e=0;e<lpDetailsScript->GetNumElements();e++)
        {
            elem = lpDetailsScript->GetData(e,&elem_size);
            if(e == (lpDetailsScript->GetNumElements() - 1) )
            {
                if(elem_size > 0)
                {
                    scriptOpReturn << OP_RETURN << vector<unsigned char>(elem, elem + elem_size);
                }
                else
                {
                    scriptOpReturn << OP_RETURN;
                }
            }
            else
            {
                if(elem_size > 0)
                {
                    scriptOpReturn << vector<unsigned char>(elem, elem + elem_size) << OP_DROP;
                }                
            }
        }    
    }
    
    vector<CTxDestination> addresses;    
    
    vector<CTxDestination> fromaddresses;        
    
    if(params[0].get_str() != "*")
    {
        fromaddresses=ParseAddresses(params[0].get_str(),false,false);

        if(fromaddresses.size() != 1)
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Single from-address should be specified");                        
        }

        if( (IsMine(*pwalletMain, fromaddresses[0]) & ISMINE_SPENDABLE) != ISMINE_SPENDABLE )
        {
            throw JSONRPCError(RPC_WALLET_ADDRESS_NOT_FOUND, "Private key for from-address is not found in this wallet");                        
        }
        
        set<CTxDestination> thisFromAddresses;

        BOOST_FOREACH(const CTxDestination& fromaddress, fromaddresses)
        {
            thisFromAddresses.insert(fromaddress);
        }

        CPubKey pkey;
        if(!pwalletMain->GetKeyFromAddressBook(pkey,MC_PTP_CREATE,&thisFromAddresses))
        {
            throw JSONRPCError(RPC_INSUFFICIENT_PERMISSIONS, "from-address doesn't have create permission");                
        }   
    }
    else
    {
        CPubKey pkey;
        if(!pwalletMain->GetKeyFromAddressBook(pkey,MC_PTP_CREATE))
        {
            throw JSONRPCError(RPC_INSUFFICIENT_PERMISSIONS, "This wallet doesn't have keys with create permission");                
        }        
    }
    
    
    EnsureWalletIsUnlocked();
    LOCK (pwalletMain->cs_wallet_send);
    
    SendMoneyToSeveralAddresses(addresses, 0, wtx, lpScript, scriptOpReturn,fromaddresses);

    return wtx.GetHash().GetHex();    
}

Value createfromcmd(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 4)
        throw runtime_error("Help message not found\n");
    
    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
    if (strcmp(params[1].get_str().c_str(),"stream") == 0)
    {
        return createstreamfromcmd(params,fHelp);    
    }
    
    if (strcmp(params[1].get_str().c_str(),"upgrade") == 0)
    {
        return createupgradefromcmd(params,fHelp);    
    }
    
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid entity type, should be stream");
}

Value createcmd(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3)
        throw runtime_error("Help message not found\n");

    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
    
    Array ext_params;
    ext_params.push_back("*");
    BOOST_FOREACH(const Value& value, params)
    {
        ext_params.push_back(value);
    }
    
    return createfromcmd(ext_params,fHelp);    
}

Value publish(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");
    
    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
    
    Array ext_params;
    ext_params.push_back("*");
    BOOST_FOREACH(const Value& value, params)
    {
        ext_params.push_back(value);
    }
    
    return publishfrom(ext_params,fHelp);    
}

Value publishfrom(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
       
    if(params[2].get_str() == "*")
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid item-key-string: *");                
    }
    
    mc_Script *lpScript=mc_gState->m_TmpBuffers->m_RpcScript3;
    lpScript->Clear();
    mc_EntityDetails stream_entity;
    parseStreamIdentifier(params[1],&stream_entity);           
               
    
    // Wallet comments
    CWalletTx wtx;
            
    vector<CTxDestination> addresses;    
    
    vector<CTxDestination> fromaddresses;        
    
    if(params[0].get_str() != "*")
    {
        fromaddresses=ParseAddresses(params[0].get_str(),false,false);

        if(fromaddresses.size() != 1)
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Single from-address should be specified");                        
        }
        if( (IsMine(*pwalletMain, fromaddresses[0]) & ISMINE_SPENDABLE) != ISMINE_SPENDABLE )
        {
            throw JSONRPCError(RPC_WALLET_ADDRESS_NOT_FOUND, "Private key for from-address is not found in this wallet");                        
        }
    }

    FindAddressesWithPublishPermission(fromaddresses,&stream_entity);
        
    if(params[2].get_str().size() > MC_ENT_MAX_ITEM_KEY_SIZE)
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Item key is too long");                                                                                                    
    }
    
    mc_Script *lpDetailsScript=mc_gState->m_TmpBuffers->m_RpcScript1;
    lpDetailsScript->Clear();
        

    bool fIsHex;
    vector<unsigned char> dataData(ParseHex(params[3].get_str().c_str(),fIsHex));    
    if(!fIsHex)
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Item data should be hexadecimal string");                                                                                                    
    }
    
    lpDetailsScript->Clear();
    lpDetailsScript->SetEntity(stream_entity.GetTxID()+MC_AST_SHORT_TXID_OFFSET);
    lpDetailsScript->SetItemKey((unsigned char*)params[2].get_str().c_str(),params[2].get_str().size());

    lpDetailsScript->AddElement();
    if(dataData.size())
    {
        lpDetailsScript->SetData(&dataData[0],dataData.size());
    }

    size_t elem_size;
    const unsigned char *elem;
    CScript scriptOpReturn=CScript();
    
    for(int e=0;e<lpDetailsScript->GetNumElements();e++)
    {
        elem = lpDetailsScript->GetData(e,&elem_size);
        if(e == (lpDetailsScript->GetNumElements() - 1) )
        {
            if(elem_size > 0)
            {
                scriptOpReturn << OP_RETURN << vector<unsigned char>(elem, elem + elem_size);
            }
            else
            {
                scriptOpReturn << OP_RETURN;
            }
        }
        else
        {
            if(elem_size > 0)
            {
                scriptOpReturn << vector<unsigned char>(elem, elem + elem_size) << OP_DROP;
            }                
        }
    }    
    
    
    lpScript->Clear();
         
    EnsureWalletIsUnlocked();
    LOCK (pwalletMain->cs_wallet_send);
    
    SendMoneyToSeveralAddresses(addresses, 0, wtx, lpScript, scriptOpReturn,fromaddresses);

    return wtx.GetHash().GetHex();    
}

Value subscribe(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error("Help message not found\n");

    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
    if((mc_gState->m_WalletMode & MC_WMD_TXS) == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported with this wallet version. To get this functionality, run \"multichaind -walletdbversion=2 -rescan\" ");        
    }   
       
    // Whether to perform rescan after import
    bool fRescan = true;
    if (params.size() > 1)
        fRescan = params[1].get_bool();

    vector<mc_EntityDetails> inputEntities;
    vector<string> inputStrings;
    if(params[0].type() == str_type)
    {
        inputStrings.push_back(params[0].get_str());
    }
    else
    {    
        inputStrings=ParseStringList(params[0]);
    }
    
    for(int is=0;is<(int)inputStrings.size();is++)
    {
        mc_EntityDetails entity_to_subscribe;
        Value param=inputStrings[is];
        ParseEntityIdentifier(param,&entity_to_subscribe, MC_ENT_TYPE_ANY);           
        inputEntities.push_back(entity_to_subscribe);
    }
    
    bool fNewFound=false;
    for(int is=0;is<(int)inputStrings.size();is++)
    {
        mc_EntityDetails* lpEntity;
        lpEntity=&inputEntities[is];
        
        mc_TxEntity entity;
        if(lpEntity->GetEntityType() == MC_ENT_TYPE_STREAM)
        {
            entity.Zero();
            memcpy(entity.m_EntityID,lpEntity->GetTxID()+MC_AST_SHORT_TXID_OFFSET,MC_AST_SHORT_TXID_SIZE);
            entity.m_EntityType=MC_TET_STREAM | MC_TET_CHAINPOS;
            if(pwalletTxsMain->AddEntity(&entity,MC_EFL_NOT_IN_SYNC) != MC_ERR_FOUND)
            {
                entity.m_EntityType=MC_TET_STREAM | MC_TET_TIMERECEIVED;
                pwalletTxsMain->AddEntity(&entity,MC_EFL_NOT_IN_SYNC);
                entity.m_EntityType=MC_TET_STREAM_KEY | MC_TET_CHAINPOS;
                pwalletTxsMain->AddEntity(&entity,MC_EFL_NOT_IN_SYNC);
                entity.m_EntityType=MC_TET_STREAM_KEY | MC_TET_TIMERECEIVED;
                pwalletTxsMain->AddEntity(&entity,MC_EFL_NOT_IN_SYNC);
                entity.m_EntityType=MC_TET_STREAM_PUBLISHER | MC_TET_CHAINPOS;
                pwalletTxsMain->AddEntity(&entity,MC_EFL_NOT_IN_SYNC);
                entity.m_EntityType=MC_TET_STREAM_PUBLISHER | MC_TET_TIMERECEIVED;
                pwalletTxsMain->AddEntity(&entity,MC_EFL_NOT_IN_SYNC);
                fNewFound=true;
            }
        }

        if(lpEntity->GetEntityType() == MC_ENT_TYPE_ASSET)
        {
            entity.Zero();
            memcpy(entity.m_EntityID,lpEntity->GetShortRef(),mc_gState->m_NetworkParams->m_AssetRefSize);
            entity.m_EntityType=MC_TET_ASSET | MC_TET_CHAINPOS;
            if(pwalletTxsMain->AddEntity(&entity,MC_EFL_NOT_IN_SYNC) != MC_ERR_FOUND)
            {
                entity.m_EntityType=MC_TET_ASSET | MC_TET_TIMERECEIVED;
                pwalletTxsMain->AddEntity(&entity,MC_EFL_NOT_IN_SYNC);
                fNewFound=true;
            }
        }
    }
    
    if (fRescan && fNewFound)
    {
        pwalletMain->ScanForWalletTransactions(chainActive.Genesis(), true, true);
    }

    return Value::null;
}


Value unsubscribe(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error("Help message not found\n");

    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
    if((mc_gState->m_WalletMode & MC_WMD_TXS) == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported with this wallet version. To get this functionality, run \"multichaind -walletdbversion=2 -rescan\" ");        
    }   
       

    vector<mc_EntityDetails> inputEntities;
    vector<string> inputStrings;
    if(params[0].type() == str_type)
    {
        inputStrings.push_back(params[0].get_str());
    }
    else
    {    
        inputStrings=ParseStringList(params[0]);
    }
    
    for(int is=0;is<(int)inputStrings.size();is++)
    {
        mc_EntityDetails entity_to_subscribe;
        Value param=inputStrings[is];
        ParseEntityIdentifier(param,&entity_to_subscribe, MC_ENT_TYPE_ANY);           
        inputEntities.push_back(entity_to_subscribe);
    }
        
    mc_Buffer *streams=mc_gState->m_TmpBuffers->m_RpcBuffer1;
    streams->Initialize(sizeof(mc_TxEntity),sizeof(mc_TxEntity),MC_BUF_MODE_DEFAULT);
    
    
    bool fNewFound=false;
    for(int is=0;is<(int)inputStrings.size();is++)
    {
        mc_EntityDetails* lpEntity;
        lpEntity=&inputEntities[is];
    
        mc_TxEntity entity;
        if(lpEntity->GetEntityType() == MC_ENT_TYPE_STREAM)
        {
            entity.Zero();
            memcpy(entity.m_EntityID,lpEntity->GetTxID()+MC_AST_SHORT_TXID_OFFSET,MC_AST_SHORT_TXID_SIZE);
            entity.m_EntityType=MC_TET_STREAM | MC_TET_CHAINPOS;
            streams->Add(&entity,NULL);
            entity.m_EntityType=MC_TET_STREAM | MC_TET_TIMERECEIVED;
            streams->Add(&entity,NULL);
            entity.m_EntityType=MC_TET_STREAM_KEY | MC_TET_CHAINPOS;
            streams->Add(&entity,NULL);
            entity.m_EntityType=MC_TET_STREAM_KEY | MC_TET_TIMERECEIVED;
            streams->Add(&entity,NULL);
            entity.m_EntityType=MC_TET_STREAM_PUBLISHER | MC_TET_CHAINPOS;
            streams->Add(&entity,NULL);
            entity.m_EntityType=MC_TET_STREAM_PUBLISHER | MC_TET_TIMERECEIVED;
            streams->Add(&entity,NULL);
            fNewFound=true;
        }

        if(lpEntity->GetEntityType() == MC_ENT_TYPE_ASSET)
        {
            entity.Zero();
            memcpy(entity.m_EntityID,lpEntity->GetShortRef(),mc_gState->m_NetworkParams->m_AssetRefSize);
            entity.m_EntityType=MC_TET_ASSET | MC_TET_CHAINPOS;
            streams->Add(&entity,NULL);
            entity.m_EntityType=MC_TET_ASSET | MC_TET_TIMERECEIVED;
            streams->Add(&entity,NULL);
            fNewFound=true;
        }
    }

    if(fNewFound)
    {
        if(pwalletTxsMain->Unsubscribe(streams))
        {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't unsubscribe from stream");                                    
        }
    }

    return Value::null;
}

Value getstreamitem(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error("Help message not found\n");
   
    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
    if((mc_gState->m_WalletMode & MC_WMD_TXS) == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported with this wallet version. For full streams functionality, run \"multichaind -walletdbversion=2 -rescan\" ");        
    }   
           
    mc_EntityDetails stream_entity;
    parseStreamIdentifier(params[0],&stream_entity);           
    
    mc_TxEntityStat entStat;
    entStat.Zero();
    memcpy(&entStat,stream_entity.GetTxID()+MC_AST_SHORT_TXID_OFFSET,MC_AST_SHORT_TXID_SIZE);
    entStat.m_Entity.m_EntityType=MC_TET_STREAM;
    entStat.m_Entity.m_EntityType |= MC_TET_CHAINPOS;

    if(!pwalletTxsMain->FindEntity(&entStat))
    {
        throw JSONRPCError(RPC_NOT_SUBSCRIBED, "Not subscribed to this stream");                                
    }
    
    
    uint256 hash = ParseHashV(params[1], "parameter 2");
    
    bool verbose=false;
    
    if (params.size() > 2)    
    {
        verbose=paramtobool(params[2]);
    }
    
    const CWalletTx& wtx=pwalletTxsMain->GetWalletTx(hash,NULL,NULL);
    
    Object entry=StreamItemEntry(wtx,stream_entity.GetTxID()+MC_AST_SHORT_TXID_OFFSET,verbose);    
    
    if(entry.size() == 0)
    {
        throw JSONRPCError(RPC_TX_NOT_FOUND, "This transaction was not found in this stream");                
    }
    
    return entry;
}

Value liststreamitems(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 5)
        throw runtime_error("Help message not found\n");

    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
    if((mc_gState->m_WalletMode & MC_WMD_TXS) == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported with this wallet version. For full streams functionality, run \"multichaind -walletdbversion=2 -rescan\" ");        
    }   
           
    mc_TxEntityStat entStat;
    
    mc_EntityDetails stream_entity;
    parseStreamIdentifier(params[0],&stream_entity);           

    int count,start;
    bool verbose=false;
    
    if (params.size() > 1)    
    {
        verbose=paramtobool(params[1]);
    }
    
    count=10;
    if (params.size() > 2)    
    {
        count=paramtoint(params[2],true,0,"Invalid count");
    }
    start=-count;
    if (params.size() > 3)    
    {
        start=paramtoint(params[3],false,0,"Invalid start");
    }
    
    bool fLocalOrdering = false;
    if (params.size() > 4)
        fLocalOrdering = params[4].get_bool();
    
    entStat.Zero();
    memcpy(&entStat,stream_entity.GetTxID()+MC_AST_SHORT_TXID_OFFSET,MC_AST_SHORT_TXID_SIZE);
    entStat.m_Entity.m_EntityType=MC_TET_STREAM;
    if(fLocalOrdering)
    {
        entStat.m_Entity.m_EntityType |= MC_TET_TIMERECEIVED;
    }
    else
    {
        entStat.m_Entity.m_EntityType |= MC_TET_CHAINPOS;
    }
    if(!pwalletTxsMain->FindEntity(&entStat))
    {
        throw JSONRPCError(RPC_NOT_SUBSCRIBED, "Not subscribed to this stream");                                
    }
    
    mc_Buffer *entity_rows=mc_gState->m_TmpBuffers->m_RpcEntityRows;
    entity_rows->Clear();
    
    mc_AdjustStartAndCount(&count,&start,entStat.m_LastPos);
    
    Array retArray;
    pwalletTxsMain->GetList(&entStat.m_Entity,start+1,count,entity_rows);
    
    for(int i=0;i<entity_rows->GetCount();i++)
    {
        mc_TxEntityRow *lpEntTx;
        lpEntTx=(mc_TxEntityRow*)entity_rows->GetRow(i);
        uint256 hash;
        memcpy(&hash,lpEntTx->m_TxId,MC_TDB_TXID_SIZE);
        const CWalletTx& wtx=pwalletTxsMain->GetWalletTx(hash,NULL,NULL);
        Object entry=StreamItemEntry(wtx,stream_entity.GetTxID()+MC_AST_SHORT_TXID_OFFSET,verbose);
        if(entry.size())
        {
            retArray.push_back(entry);                                
        }
    }
    
    return retArray;
}

void getTxsForBlockRange(vector <uint256>& txids,mc_TxEntity *entity,int height_from,int height_to,mc_Buffer *entity_rows)
{
    int first_item,last_item,count,i;
    
    last_item=pwalletTxsMain->GetBlockItemIndex(entity,height_to);
    if(last_item)
    {
        first_item=pwalletTxsMain->GetBlockItemIndex(entity,height_from-1)+1;
        count=last_item-first_item+1;
        if(count > 0)
        {
            pwalletTxsMain->GetList(entity,first_item,count,entity_rows);
            
            mc_TxEntityRow *lpEntTx;
            uint256 hash;
            for(i=0;i<count;i++)
            {
                lpEntTx=(mc_TxEntityRow*)entity_rows->GetRow(i);
                memcpy(&hash,lpEntTx->m_TxId,MC_TDB_TXID_SIZE);
                txids.push_back(hash);
            }
        }        
    }
}

Value liststreamblockitems(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 5)
        throw runtime_error("Help message not found\n");

    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
    if((mc_gState->m_WalletMode & MC_WMD_TXS) == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported with this wallet version. For full streams functionality, run \"multichaind -walletdbversion=2 -rescan\" ");        
    }   
           
    mc_TxEntityStat entStat;
    
    mc_EntityDetails stream_entity;
    parseStreamIdentifier(params[0],&stream_entity);           

    int count,start;
    bool verbose=false;
    
    if (params.size() > 2)    
    {
        verbose=paramtobool(params[2]);
    }
    
    count=2147483647;
    if (params.size() > 3)    
    {
        count=paramtoint(params[3],true,0,"Invalid count");
    }
    start=-count;
    if (params.size() > 4)    
    {
        start=paramtoint(params[4],false,0,"Invalid start");
    }
    
    entStat.Zero();
    memcpy(&entStat,stream_entity.GetTxID()+MC_AST_SHORT_TXID_OFFSET,MC_AST_SHORT_TXID_SIZE);
    entStat.m_Entity.m_EntityType=MC_TET_STREAM;
    entStat.m_Entity.m_EntityType |= MC_TET_CHAINPOS;
    if(!pwalletTxsMain->FindEntity(&entStat))
    {
        throw JSONRPCError(RPC_NOT_SUBSCRIBED, "Not subscribed to this stream");                                
    }
    
    
    vector <int> heights=ParseBlockSetIdentifier(params[1]);
    vector <uint256> txids;
    
    Array retArray;
    if(heights.size() == 0)
    {
        return retArray;
    }
    
    int height_from,height_to;
    height_from=heights[0];
    height_to=heights[0];

    mc_Buffer *entity_rows=mc_gState->m_TmpBuffers->m_RpcEntityRows;
    entity_rows->Clear();
    
    for(unsigned int i=1;i<heights.size();i++)
    {
        if(heights[i] > height_to + 1)
        {
            getTxsForBlockRange(txids,&entStat.m_Entity,height_from,height_to,entity_rows);
            height_from=heights[i];
        }
        height_to=heights[i];
    }
    
    
    getTxsForBlockRange(txids,&entStat.m_Entity,height_from,height_to,entity_rows);
    
    mc_AdjustStartAndCount(&count,&start,txids.size());
    
    for(int i=start;i<start+count;i++)
    {
        const CWalletTx& wtx=pwalletTxsMain->GetWalletTx(txids[i],NULL,NULL);
        Object entry=StreamItemEntry(wtx,stream_entity.GetTxID()+MC_AST_SHORT_TXID_OFFSET,verbose);
        if(entry.size())
        {
            retArray.push_back(entry);                                
        }
    }
    
    return retArray;
}


void getSubKeyEntityFromKey(string str,mc_TxEntityStat entStat,mc_TxEntity *entity)
{
    if(str == "*")
    {
        return;
    }
    uint160 key_string_hash;
    uint160 stream_subkey_hash;
    key_string_hash=Hash160(str.begin(),str.end());
    mc_GetCompoundHash160(&stream_subkey_hash,entStat.m_Entity.m_EntityID,&key_string_hash);
    memcpy(entity->m_EntityID,&stream_subkey_hash,MC_TDB_ENTITY_ID_SIZE);
    entity->m_EntityType=entStat.m_Entity.m_EntityType | MC_TET_SUBKEY;    
}

void getSubKeyEntityFromPublisher(string str,mc_TxEntityStat entStat,mc_TxEntity *entity)
{
    if(str == "*")
    {
        return;
    }
    uint160 stream_subkey_hash;
    CBitcoinAddress address(str);
    if (!address.IsValid())
    {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");            
    }
    CTxDestination dest=address.Get();
    CKeyID *lpKeyID=boost::get<CKeyID> (&dest);
    CScriptID *lpScriptID=boost::get<CScriptID> (&dest);

    
    if ((lpKeyID == NULL) && (lpScriptID == NULL) )
    {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");                    
    }

    if(lpKeyID)
    {
        mc_GetCompoundHash160(&stream_subkey_hash,entStat.m_Entity.m_EntityID,lpKeyID);        
    }
    else
    {
        mc_GetCompoundHash160(&stream_subkey_hash,entStat.m_Entity.m_EntityID,lpScriptID);                
    }

    memcpy(entity->m_EntityID,&stream_subkey_hash,MC_TDB_ENTITY_ID_SIZE);
    entity->m_EntityType=entStat.m_Entity.m_EntityType | MC_TET_SUBKEY;    
}

Value liststreamkeyitems(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 6)
        throw runtime_error("Help message not found\n");

    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
    if((mc_gState->m_WalletMode & MC_WMD_TXS) == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported with this wallet version. For full streams functionality, run \"multichaind -walletdbversion=2 -rescan\" ");        
    }   
    
    if(params[1].get_str() == "*")
    {
        int count=0;
        Array ext_params;
        BOOST_FOREACH(const Value& value, params)
        {
            if(count != 1)
            {
                ext_params.push_back(value);
            }
            count++;
        }
    
        return liststreamitems(ext_params,fHelp);            
    }
           
    mc_TxEntityStat entStat;
    mc_TxEntity entity;
    
    mc_EntityDetails stream_entity;
    parseStreamIdentifier(params[0],&stream_entity);           

    int count,start;
    bool verbose=false;
    
    if (params.size() > 2)    
    {
        verbose=paramtobool(params[2]);
    }

    count=10;
    if (params.size() > 3)    
    {
        count=paramtoint(params[3],true,0,"Invalid count");
    }
    start=-count;
    if (params.size() > 4)    
    {
        start=paramtoint(params[4],false,0,"Invalid start");
    }
        
    bool fLocalOrdering = false;
    if (params.size() > 5)
        fLocalOrdering = params[5].get_bool();
    
    entStat.Zero();
    memcpy(&entStat,stream_entity.GetTxID()+MC_AST_SHORT_TXID_OFFSET,MC_AST_SHORT_TXID_SIZE);
    entStat.m_Entity.m_EntityType=MC_TET_STREAM_KEY;
    if(fLocalOrdering)
    {
        entStat.m_Entity.m_EntityType |= MC_TET_TIMERECEIVED;
    }
    else
    {
        entStat.m_Entity.m_EntityType |= MC_TET_CHAINPOS;
    }
    if(!pwalletTxsMain->FindEntity(&entStat))
    {
        throw JSONRPCError(RPC_NOT_SUBSCRIBED, "Not subscribed to this stream");                                
    }

    getSubKeyEntityFromKey(params[1].get_str(),entStat,&entity);
    
    
    mc_Buffer *entity_rows=mc_gState->m_TmpBuffers->m_RpcEntityRows;
    entity_rows->Clear();
    
    mc_AdjustStartAndCount(&count,&start,pwalletTxsMain->GetListSize(&entity,entStat.m_Generation,NULL));
    
    Array retArray;
    pwalletTxsMain->GetList(&entity,entStat.m_Generation,start+1,count,entity_rows);
    
    for(int i=0;i<entity_rows->GetCount();i++)
    {
        mc_TxEntityRow *lpEntTx;
        lpEntTx=(mc_TxEntityRow*)entity_rows->GetRow(i);
        uint256 hash;
        memcpy(&hash,lpEntTx->m_TxId,MC_TDB_TXID_SIZE);
        const CWalletTx& wtx=pwalletTxsMain->GetWalletTx(hash,NULL,NULL);
        Object entry=StreamItemEntry(wtx,stream_entity.GetTxID()+MC_AST_SHORT_TXID_OFFSET,verbose);
        if(entry.size())
        {
            retArray.push_back(entry);                                
        }
    }
    
    return retArray;
}


Value liststreampublisheritems(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 6)
        throw runtime_error("Help message not found\n");

    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
    if((mc_gState->m_WalletMode & MC_WMD_TXS) == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported with this wallet version. For full streams functionality, run \"multichaind -walletdbversion=2 -rescan\" ");        
    }   
           
    if(params[1].get_str() == "*")
    {
        int count=0;
        Array ext_params;
        BOOST_FOREACH(const Value& value, params)
        {
            if(count != 1)
            {
                ext_params.push_back(value);
            }
            count++;
        }
    
        return liststreamitems(ext_params,fHelp);            
    }
    
    mc_TxEntityStat entStat;
    mc_TxEntity entity;
    uint160 stream_subkey_hash;
    
    mc_EntityDetails stream_entity;
    parseStreamIdentifier(params[0],&stream_entity);           

    int count,start;
    bool verbose=false;
    
    if (params.size() > 2)    
    {
        verbose=paramtobool(params[2]);
    }

    count=10;
    if (params.size() > 3)    
    {
        count=paramtoint(params[3],true,0,"Invalid count");
    }
    start=-count;
    if (params.size() > 4)    
    {
        start=paramtoint(params[4],false,0,"Invalid start");
    }

    bool fLocalOrdering = false;
    if (params.size() > 5)
        fLocalOrdering = params[5].get_bool();
    
    entStat.Zero();
    memcpy(&entStat,stream_entity.GetTxID()+MC_AST_SHORT_TXID_OFFSET,MC_AST_SHORT_TXID_SIZE);
    entStat.m_Entity.m_EntityType=MC_TET_STREAM_PUBLISHER;
    if(fLocalOrdering)
    {
        entStat.m_Entity.m_EntityType |= MC_TET_TIMERECEIVED;
    }
    else
    {
        entStat.m_Entity.m_EntityType |= MC_TET_CHAINPOS;
    }
    if(!pwalletTxsMain->FindEntity(&entStat))
    {
        throw JSONRPCError(RPC_NOT_SUBSCRIBED, "Not subscribed to this stream");                                
    }

    getSubKeyEntityFromPublisher(params[1].get_str(),entStat,&entity);
    
    mc_Buffer *entity_rows=mc_gState->m_TmpBuffers->m_RpcEntityRows;
    entity_rows->Clear();
    
    mc_AdjustStartAndCount(&count,&start,pwalletTxsMain->GetListSize(&entity,entStat.m_Generation,NULL));
    
    Array retArray;
    pwalletTxsMain->GetList(&entity,entStat.m_Generation,start+1,count,entity_rows);
    
    for(int i=0;i<entity_rows->GetCount();i++)
    {
        mc_TxEntityRow *lpEntTx;
        lpEntTx=(mc_TxEntityRow*)entity_rows->GetRow(i);
        uint256 hash;
        memcpy(&hash,lpEntTx->m_TxId,MC_TDB_TXID_SIZE);
        const CWalletTx& wtx=pwalletTxsMain->GetWalletTx(hash,NULL,NULL);
        Object entry=StreamItemEntry(wtx,stream_entity.GetTxID()+MC_AST_SHORT_TXID_OFFSET,verbose);
        if(entry.size())
        {
            retArray.push_back(entry);                                
        }
    }
    
    return retArray;
}

bool IsAllowedMapMode(string mode)
{
    if(mode == "list")        return true;
    if(mode == "all")        return true;
    return false;
}

//Value liststreammap_operation(mc_TxEntity *parent_entity,mc_TxEntity *subkey_entity,string subkey_string,int count, int start, string mode)
Value liststreammap_operation(mc_TxEntity *parent_entity,vector<mc_TxEntity>& inputEntities,vector<string>& inputStrings,int count, int start, string mode)
{
    mc_TxEntity entity;
    mc_TxEntityStat entStat;
    Array retArray;
    mc_Buffer *entity_rows=mc_gState->m_TmpBuffers->m_RpcEntityRows;
    mc_TxEntityRow erow;
    uint160 stream_subkey_hash;    
    int row,enitity_count;
    
    entity_rows->Clear();
    enitity_count=inputEntities.size();
    if(enitity_count == 0)
    {
        mc_AdjustStartAndCount(&count,&start,pwalletTxsMain->GetListSize(parent_entity,NULL));
        entity_rows->Clear();
        pwalletTxsMain->GetList(parent_entity,start+1,count,entity_rows);
        enitity_count=entity_rows->GetCount();
    }
    else
    {
        mc_AdjustStartAndCount(&count,&start,enitity_count);       
        enitity_count=count;
    }
    
    entStat.Zero();
    if(enitity_count)
    {
        memcpy(&entStat,parent_entity,sizeof(mc_TxEntity));
        pwalletTxsMain->FindEntity(&entStat);
    }
    
    for(int i=0;i<enitity_count;i++)
    {
        mc_TxEntityRow *lpEntTx;
        string key_string;
        if(entity_rows->GetCount())
        {
            lpEntTx=(mc_TxEntityRow*)entity_rows->GetRow(i);
            key_string=pwalletTxsMain->GetSubKey(lpEntTx->m_TxId, NULL,NULL);
            entity.Zero();
            mc_GetCompoundHash160(&stream_subkey_hash,parent_entity->m_EntityID,lpEntTx->m_TxId);
            memcpy(entity.m_EntityID,&stream_subkey_hash,MC_TDB_ENTITY_ID_SIZE);
            entity.m_EntityType=parent_entity->m_EntityType | MC_TET_SUBKEY;
        }
        else
        {
            memcpy(&entity,&(inputEntities[i+start]),sizeof(mc_TxEntity));
            key_string=inputStrings[i+start];
        }
        
        int total,confirmed;
        total=pwalletTxsMain->GetListSize(&entity,entStat.m_Generation,&confirmed);
        
        Object all_entry;
        int shift=total-1;
        if(shift == 0)
        {
            shift=1;
        }
        if((parent_entity->m_EntityType & MC_TET_TYPE_MASK) == MC_TET_STREAM_PUBLISHER)
        {
            all_entry.push_back(Pair("publisher", key_string));                                                                                                                
        }
        else
        {
            all_entry.push_back(Pair("key", key_string));                                                                                            
        }
        all_entry.push_back(Pair("items", total));                                                                        
        all_entry.push_back(Pair("confirmed", confirmed));                                                                        
        
        if(mode == "all")
        {
            for(row=1;row<=total;row+=shift)
            {
                if( ( (row == 1) && (mode != "last") ) || ( (row == total) && (mode != "first") ) )
                {                    
                    erow.Zero();
                    memcpy(&erow.m_Entity,&entity,sizeof(mc_TxEntity));
                    erow.m_Generation=entStat.m_Generation;
                    erow.m_Pos=row;

                    if(pwalletTxsMain->GetRow(&erow) == 0)
                    {
                        uint256 hash;
                        memcpy(&hash,erow.m_TxId,MC_TDB_TXID_SIZE);
                        const CWalletTx& wtx=pwalletTxsMain->GetWalletTx(hash,NULL,NULL);

                        Value item_value;

                        item_value=StreamItemEntry(wtx,parent_entity->m_EntityID,true);
                        if(row == 1)
                        {
                            all_entry.push_back(Pair("first", item_value));                                                                        
                        }
                        if(row == total)
                        {
                            all_entry.push_back(Pair("last", item_value));                                                                        
                        }
                    }
                }
            }
        }
        retArray.push_back(all_entry);                                
    }

    return retArray;
}

Value liststreamkeys_or_publishers(const Array& params,bool is_publishers)
{
    mc_TxEntity entity;
    mc_TxEntityStat entStat;
    
    mc_EntityDetails stream_entity;
    
    parseStreamIdentifier(params[0],&stream_entity);           

    string mode="list";
    
    if (params.size() > 2)    
    {
        if(paramtobool(params[2]))
        {
            mode="all";            
        }
    }
        
    int count,start;
    count=2147483647;
    if (params.size() > 3)    
    {
        count=paramtoint(params[3],true,0,"Invalid count");
    }
    start=-count;
    if (params.size() > 4)    
    {
        start=paramtoint(params[4],false,0,"Invalid start");
    }
    
    bool fLocalOrdering = false;
    if (params.size() > 5)
        fLocalOrdering = params[5].get_bool();
    
    entStat.Zero();
    memcpy(&entStat,stream_entity.GetTxID()+MC_AST_SHORT_TXID_OFFSET,MC_AST_SHORT_TXID_SIZE);
    if(is_publishers)
    {
        entStat.m_Entity.m_EntityType=MC_TET_STREAM_PUBLISHER;                
    }
    else
    {
        entStat.m_Entity.m_EntityType=MC_TET_STREAM_KEY;        
    }
    if(fLocalOrdering)
    {
        entStat.m_Entity.m_EntityType |= MC_TET_TIMERECEIVED;
    }
    else
    {
        entStat.m_Entity.m_EntityType |= MC_TET_CHAINPOS;
    }
    
    if(!pwalletTxsMain->FindEntity(&entStat))
    {
        throw JSONRPCError(RPC_NOT_SUBSCRIBED, "Not subscribed to this stream");                                
    }

    vector<string> inputStrings;
    vector<mc_TxEntity> inputEntities;
    
    if(params.size() > 1)
    {
        if(!is_publishers && (params[1].type() == str_type) )
        {
            inputStrings.push_back(params[1].get_str());
        }
        else
        {
            inputStrings=ParseStringList(params[1]);
            if(inputStrings.size() == 0)
            {
                Array retArray;                
                return retArray;
            }
        }
        bool take_it=true;
        if( (inputStrings.size() == 1) && (inputStrings[0] == "*") )
        {
            take_it=false;
        }
        if(take_it)
        {            
            for(int is=0;is<(int)inputStrings.size();is++)
            {
                string str=inputStrings[is];
                entity.Zero();

                if(is_publishers)
                {
                    getSubKeyEntityFromPublisher(str,entStat,&entity);
                }
                else
                {
                    getSubKeyEntityFromKey(str,entStat,&entity);        
                }
                inputEntities.push_back(entity);
            }
        }
    }
    
    return liststreammap_operation(&(entStat.m_Entity),inputEntities,inputStrings,count,start,mode);        
}

Value liststreamkeys(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 6)
        throw runtime_error("Help message not found\n");
    
    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
    if((mc_gState->m_WalletMode & MC_WMD_TXS) == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported with this wallet version. For full streams functionality, run \"multichaind -walletdbversion=2 -rescan\" ");        
    }   
           
    return liststreamkeys_or_publishers(params,false);
}

Value liststreampublishers(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 6)
        throw runtime_error("Help message not found\n");
    if((mc_gState->m_WalletMode & MC_WMD_TXS) == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported with this wallet version. For full streams functionality, run \"multichaind -walletdbversion=2 -rescan\" ");        
    }   
    
    if(mc_gState->m_Features->Streams() == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported for this protocol version");        
    }
           
    return liststreamkeys_or_publishers(params,true);
}

/* AMB START */
// param1 - from-address
// param2 - wallet address
// param3 - encrypted data
// param4 - encrypted private key

Value writerecord(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    Object content;

    content.push_back(Pair("data",params[2]));
    content.push_back(Pair("encrypted-key",params[3]));

    const Value& json_data = content;
    const std::string string_data = write_string(json_data, false);
    
    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_RECORDS));
    raw_data.push_back(Pair("key", params[1]));
    raw_data.push_back(Pair("data", hex_data));

    Array ext_params;

    Object addresses;
    Array dataArray;
    dataArray.push_back(raw_data);
    ext_params.push_back(params[0]); // from-address
    ext_params.push_back(addresses); // addresses
    ext_params.push_back(dataArray); // data array

    return createrawsendfrom(ext_params, fHelp);
}

/* AMB START */
// param0 - from-address
// param1 - key
// param2 - encrypted data
// param3 - encrypted private key
Value createbulletinboard(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    Object content;

    content.push_back(Pair("data",params[2]));
    content.push_back(Pair("encrypted-key",params[3]));

    const Value& json_data = content;
    const std::string string_data = write_string(json_data, false);
    
    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_BULLETINBOARDS));
    raw_data.push_back(Pair("key", params[1]));
    raw_data.push_back(Pair("data", hex_data));

    Array ext_params;

    Object addresses;
    Array dataArray;
    dataArray.push_back(raw_data);
    ext_params.push_back(params[0]); // from-address
    ext_params.push_back(addresses); // addresses
    ext_params.push_back(dataArray); // data array

    return createrawsendfrom(ext_params, fHelp);
}

/* AMB START */
// param0 - from_address
// param1 - key
// param2 - encrypted data
Value writebulletinboardentry(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Object content;

    content.push_back(Pair("data",params[2]));

    const Value& json_data = content;
    const std::string string_data = write_string(json_data, false);
    
    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_BULLETINBOARDS));
    raw_data.push_back(Pair("key", params[1]));
    raw_data.push_back(Pair("data", hex_data));

    Array ext_params;

    Object addresses;
    Array dataArray;
    dataArray.push_back(raw_data);
    ext_params.push_back(params[0]); // from-address
    ext_params.push_back(addresses); // addresses
    ext_params.push_back(dataArray); // data array

    return createrawsendfrom(ext_params, fHelp);
}

/* AMB START */
// param0 - from_address
// param1 - key
Value censormessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error("Help message not found\n");

    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_CENSOREDMESSAGES));
    raw_data.push_back(Pair("key", params[1]));

    Array ext_params;

    Object addresses;
    Array dataArray;
    dataArray.push_back(raw_data);
    ext_params.push_back(params[0]); // from-address
    ext_params.push_back(addresses); // addresses
    ext_params.push_back(dataArray); // data array

    return createrawsendfrom(ext_params, fHelp);
}

// param1 - from-address
// param2 - previous stream id
// param3 - encrypted data
// param4 - type
Value writeannotatedrecord(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    Array pub_params;
    pub_params.push_back(STREAM_RECORDS);
    pub_params.push_back(params[1]);
    Object result = getstreamitem(pub_params, false).get_obj();

    if (result.size() > 0) {
        std::string publisher_item = result[0].value_.get_array().front().get_str();
            if (strcmp(publisher_item.c_str(), params[0].get_str().c_str()) != 0) {
                throw runtime_error("Address is not the previous publisher");
        }
    }

    Object content;

    content.push_back(Pair("type",params[3]));
    content.push_back(Pair("data",params[2]));

    const Value& json_data = content;
    const std::string string_data = write_string(json_data, false);
    
    std::string hex_data = HexStr(string_data.begin(), string_data.end());
    
    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_RECORDS));
    raw_data.push_back(Pair("key", params[1]));
    raw_data.push_back(Pair("data", hex_data));

    Array ext_params;

    Object addresses;
    Array dataArray;
    dataArray.push_back(raw_data);
    ext_params.push_back(params[0]); // from-address
    ext_params.push_back(addresses); // addresses
    ext_params.push_back(dataArray); // data array

    return createrawsendfrom(ext_params, fHelp);
}

// param1 - from-address
// param2 - previous stream id
// param3 - encrypted data
Value annotaterecord(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;

    BOOST_FOREACH(const Value& value, params)
    {
        ext_params.push_back(value);
    }
    ext_params.push_back("annotate");
    
    return writeannotatedrecord(ext_params, fHelp);
}

// param1 - from-address
// param2 - previous stream id
// param3 - encrypted data
Value revokerecord(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");


    Array ext_params;

    BOOST_FOREACH(const Value& value, params)
    {
        ext_params.push_back(value);
    }
    ext_params.push_back("revoke");

    return writeannotatedrecord(ext_params, fHelp);
}

// param1 - badge creator
// param2 - badge data
// key of all badges = rootbadges

Value createbadge(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error("Help message not found\n");

    if (haspermission(params[0].get_str(), "mine"))
    {
        Array ext_params;
        Object data;

        data.push_back(Pair("data",params[1]));

        const Value& json_data = data;
        const std::string string_data = write_string(json_data, false);

        std::string hex_data = HexStr(string_data.begin(), string_data.end());

        ext_params.push_back(params[0]); // badge creator
        ext_params.push_back(STREAM_BADGES); // stream for creating badges
        ext_params.push_back("rootbadges"); // key for all badges
        ext_params.push_back(hex_data);

        std::string badge_txid = publishfrom(ext_params, fHelp).get_str();

        Array issuer_params;
        issuer_params.push_back(params[0]);
        issuer_params.push_back(badge_txid);
        issuer_params.push_back(params[0]);

        grantbadgeissuerpermission(issuer_params, fHelp);
        return badge_txid;
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }
}

bool isbadgecreator(std::string address, std::string transaction_id)
{
    Array params;
    params.push_back(STREAM_BADGES);
    params.push_back(transaction_id);
    Object result = getstreamitem(params, false).get_obj();

    if (result.size() > 0) {
        std::string firstBadgePublisherString = result[0].value_.get_array().front().get_str();
        if ( strcmp(firstBadgePublisherString.c_str(), address.c_str()) == 0) {
            return true;
        }
    }

    return false;
}

// param1 - badge creator
// param2 - badge transaction id found in rootbadges
// param3 - badge data

Value updatebadge(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    if (haspermission(params[0].get_str(), "mine") && isbadgecreator(params[0].get_str(), params[1].get_str()))
    {
        Array ext_params;
        Object data;

        data.push_back(Pair("data",params[2]));

        const Value& json_data = data;
        const std::string string_data = write_string(json_data, false);

        std::string hex_data = HexStr(string_data.begin(), string_data.end());

        ext_params.push_back(params[0]); // badge creator
        ext_params.push_back(STREAM_BADGES); // stream for creating badges
        ext_params.push_back(params[1]); // badge identifier
        ext_params.push_back(hex_data);

        return publishfrom(ext_params, fHelp);
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }
}

// param1 - badge creator
// param2 - badge receiver
// param3 - badge transaction id found in rootbadges
// param4 - badge notes
// param5 - badge action [ issue | revoke ]

Value writeissuedbadges(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 5)
        throw runtime_error("Help message not found\n");

    Array keyBadgeParams;
    Array keyAddressParams;

    Object keyBadgeData;
    Object keyAddressData;

    //data preparation for when key is the badge txid
    keyBadgeData.push_back(Pair("address",params[1])); // badge receiver
    keyBadgeData.push_back(Pair("notes",params[3])); // badge notes
    keyBadgeData.push_back(Pair("action",params[4])); // badge action

    const Value& keyBadgeJsonData = keyBadgeData;
    const std::string keyBadgeStringData = write_string(keyBadgeJsonData, false);

    std::string keyBadgeHexData = HexStr(keyBadgeStringData.begin(), keyBadgeStringData.end());

    keyBadgeParams.push_back(params[0]); // badge creator
    keyBadgeParams.push_back(STREAM_ISSUEDBADGES); // stream for issue badges
    keyBadgeParams.push_back(params[2]); // badge identifier
    keyBadgeParams.push_back(keyBadgeHexData);

    //data preparation for when key is the badge receiver's address
    keyAddressData.push_back(Pair("badge", params[2]));
    keyAddressData.push_back(Pair("notes",params[3])); // badge notes
    keyAddressData.push_back(Pair("action",params[4])); // badge action

    const Value& keyAddressJsonData = keyAddressData;
    const std::string keyAddressStringData = write_string(keyAddressJsonData, false);

    std::string keyAddressHexData = HexStr(keyAddressStringData.begin(), keyAddressStringData.end());

    keyAddressParams.push_back(params[0]); // badge creator
    keyAddressParams.push_back(STREAM_ISSUEDBADGES); // stream for badge issuers
    keyAddressParams.push_back(params[1]); // badge receiver's address
    keyAddressParams.push_back(keyAddressHexData);

    publishfrom(keyAddressParams, fHelp);
    return publishfrom(keyBadgeParams, fHelp);
}

// param1 - badge creator
// param2 - badge receiver
// param3 - badge transaction id found in rootbadges
// param4 - badge notes

Value issuebadge(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    Array ext_params;

    if (haspermission(params[0].get_str(), "mine") && isbadgecreator(params[0].get_str(), params[2].get_str()))
    {
        BOOST_FOREACH(const Value& value, params)
        {
            ext_params.push_back(value);
        }
        ext_params.push_back("issue");
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }

    return writeissuedbadges(ext_params, fHelp);
}

// param1 - badge creator
// param2 - badge receiver
// param3 - badge transaction id found in rootbadges
// param4 - badge notes

Value revokebadge(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    Array ext_params;

    if (haspermission(params[0].get_str(), "mine") && isbadgecreator(params[0].get_str(), params[2].get_str()))
    {
        BOOST_FOREACH(const Value& value, params)
        {
            ext_params.push_back(value);
        }
        ext_params.push_back("revoke");
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }

    return writeissuedbadges(ext_params, fHelp);
}

bool isbadgeissuer(std::string creator_address, std::string issuer_address, std::string transaction_id) {
    Array params;
    params.push_back(STREAM_BADGEISSUERS);
    params.push_back(issuer_address);
    Array results = liststreamkeyitems(params, false).get_array();

    bool isBadgeIssuer = false;

    if (results.size() > 0) {
        BOOST_FOREACH(const Value& badge, results) {
            Object badgeObject = badge.get_obj();

            std::string publisher = badgeObject[0].value_.get_array().front().get_str();

            if ( strcmp(publisher.c_str(), creator_address.c_str()) == 0 ) {
                std::string hex_data = badgeObject[2].value_.get_str();
                std::string json_data = HexToStr(hex_data);
                Value data;
                read_string(json_data, data);
                Object dataObject = data.get_obj();

                std::string badgeTxId = dataObject[0].value_.get_str();
                std::string badgePermission = dataObject[1].value_.get_str();

                if (strcmp(badgeTxId.c_str(), transaction_id.c_str()) == 0) {
                    if (strcmp(badgePermission.c_str(), "grant") == 0) {
                        isBadgeIssuer = true;
                    }
                    else {
                        isBadgeIssuer = false;
                    }
                }
            }
        }
    }

    return isBadgeIssuer;
}

// param1 - badge creator
// param2 - badge receiver
// param3 - badge transaction id found in rootbadges
// param4 - badge notes
// param5 - badge action [ issue | revoke ]
// param6 - issue badge requestor

Value requestissuebadge(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 6)
        throw runtime_error("Help message not found\n");

    if (isbadgeissuer(params[0].get_str(), params[5].get_str(), params[2].get_str())) {
        Object data;

        data.push_back(Pair("receiver",params[1])); // badge receiver
        data.push_back(Pair("creator",params[0])); // badge creator
        data.push_back(Pair("notes",params[3])); // badge notes
        data.push_back(Pair("action",params[4])); // badge action
        data.push_back(Pair("requestor",params[5])); // badge requestor

        const Value& json_data = data;
        const std::string string_data = write_string(json_data, false);

        std::string hex_data = HexStr(string_data.begin(), string_data.end());

        Object raw_data;
        raw_data.push_back(Pair("for", STREAM_ISSUEBADGEREQUESTS));
        raw_data.push_back(Pair("key", params[2])); // badge identifier
        raw_data.push_back(Pair("data", hex_data));

        Array ext_params;

        Object addresses;
        Array dataArray;
        dataArray.push_back(raw_data);
        ext_params.push_back(params[5]); // badge requestor
        ext_params.push_back(addresses); // addresses
        ext_params.push_back(dataArray); // data array

        return createrawsendfrom(ext_params, fHelp);
    }
    else {
        throw runtime_error("Address is not an issuer\n");
    }
}

bool isbadgerequestproccessed(std::string creator_address, std::string request_txid)
{
    Array params;
    params.push_back(STREAM_PROCESSISSUEBADGEREQUESTS);
    params.push_back(request_txid);
    Array results = liststreamkeyitems(params, false).get_array();

    bool processed = false;

    if (results.size() > 0)
    {
        BOOST_FOREACH(const Value& value, results) {
            Object object = value.get_obj();

            std::string publisher = object[0].value_.get_array().front().get_str();

            if ( strcmp(publisher.c_str(), creator_address.c_str()) == 0 )
            {
                processed = true;
                break;
            }
        }
    }

    return processed;
}

// param1 - badge creator

Value processrequestissuebadge(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error("Help message not found\n");

    Array badge_params;
    badge_params.push_back(STREAM_BADGES);
    badge_params.push_back(params[0].get_str());
    Array badge_results = liststreampublisheritems(badge_params, false).get_array();

    BOOST_FOREACH(const Value& badge_value, badge_results)
    {
        Object badge = badge_value.get_obj();
        std::string key = badge[1].value_.get_str();

        if ( strcmp(key.c_str(), "rootbadges") == 0)
        {
            std::string badge_identifier = badge.back().value_.get_str();
            Array issuebadgerequest_params;
            issuebadgerequest_params.push_back(STREAM_ISSUEBADGEREQUESTS);
            issuebadgerequest_params.push_back(badge_identifier);

            Array issuebadgerequest_results = liststreamkeyitems(issuebadgerequest_params, false).get_array();

            BOOST_FOREACH(const Value& request_value, issuebadgerequest_results)
            {
                Object request = request_value.get_obj();
                std::string request_txid = request.back().value_.get_str();
                if (!isbadgerequestproccessed(params[0].get_str(), request_txid))
                {
                    std::string publisher = request[0].value_.get_array().front().get_str();

                    std::string hex_data = request[2].value_.get_str();
                    std::string json_data = HexToStr(hex_data);
                    Value data;
                    read_string(json_data, data);
                    Object data_object = data.get_obj();

                    std::string requestor = data_object[4].value_.get_str();

                    if ( (isbadgeissuer(params[0].get_str(), publisher, badge_identifier)) && (strcmp(publisher.c_str(), requestor.c_str()) == 0) )
                    {
                        std::string receiver = data_object[0].value_.get_str();
                        std::string action = data_object[3].value_.get_str();

                        Array issuebadge_params;
                        issuebadge_params.push_back(params[0].get_str());
                        issuebadge_params.push_back(receiver);
                        issuebadge_params.push_back(badge_identifier);
                        issuebadge_params.push_back("processrequestissuebadge called");

                        if (strcmp(action.c_str(), "issue") == 0)
                        {
                            issuebadge(issuebadge_params, false);
                        }
                        else if (strcmp(action.c_str(), "revoke") == 0)
                        {
                            revokebadge(issuebadge_params, false);
                        }

                        std::string string_data = "This issue/revoke badge request has been processed";
                        std::string hex_data = HexStr(string_data.begin(), string_data.end());

                        Array publish_params;
                        publish_params.push_back(params[0]);
                        publish_params.push_back(STREAM_PROCESSISSUEBADGEREQUESTS);
                        publish_params.push_back(request_txid);
                        publish_params.push_back(hex_data);

                        publishfrom(publish_params, false);
                    }
                }
            }
        }
    }

    return Value::null;
}

// param1 - badge creator
// param2 - badge transaction id found in rootbadges
// param3 - address of badge issuer
// param4 - badge issuer permission

Value writebadgeissuerpermission(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    if (haspermission(params[0].get_str(), "mine") && isbadgecreator(params[0].get_str(), params[1].get_str()))
    {
        Array keyBadgeParams;
        Array keyAddressParams;

        Object keyBadgeData;
        Object keyAddressData;

        //data preparation for when key is the badge txid
        keyBadgeData.push_back(Pair("address", params[2]));
        keyBadgeData.push_back(Pair("permission",params[3]));

        const Value& keyBadgeJsonData = keyBadgeData;
        const std::string keyBadgeStringData = write_string(keyBadgeJsonData, false);

        std::string keyBadgeHexData = HexStr(keyBadgeStringData.begin(), keyBadgeStringData.end());

        keyBadgeParams.push_back(params[0]); // badge creator
        keyBadgeParams.push_back(STREAM_BADGEISSUERS); // stream for badge issuers
        keyBadgeParams.push_back(params[1]); // badge identifier
        keyBadgeParams.push_back(keyBadgeHexData);

        //data preparation for when key is address to grant issue permissions
        keyAddressData.push_back(Pair("badge", params[1]));
        keyAddressData.push_back(Pair("permission",params[3]));

        const Value& keyAddressJsonData = keyAddressData;
        const std::string keyAddressStringData = write_string(keyAddressJsonData, false);

        std::string keyAddressHexData = HexStr(keyAddressStringData.begin(), keyAddressStringData.end());

        keyAddressParams.push_back(params[0]); // badge creator
        keyAddressParams.push_back(STREAM_BADGEISSUERS); // stream for badge issuers
        keyAddressParams.push_back(params[2]); // address to grant issue permission
        keyAddressParams.push_back(keyAddressHexData);

        publishfrom(keyAddressParams, fHelp);
        return publishfrom(keyBadgeParams, fHelp);
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }
}

// param1 - badge creator
// param2 - badge transaction id found in rootbadges
// param3 - address of badge issuer

Value grantbadgeissuerpermission(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    if (haspermission(params[0].get_str(), "mine") && isbadgecreator(params[0].get_str(), params[1].get_str()))
    {
        Array ext_params;
        Object data;

        data.push_back(Pair("data",params[2]));

        BOOST_FOREACH(const Value& value, params)
        {
            ext_params.push_back(value);
        }
        ext_params.push_back("grant");

        return writebadgeissuerpermission(ext_params, fHelp);
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }
}

// param1 - badge creator
// param2 - badge transaction id found in rootbadges
// param3 - address of badge issuer

Value revokebadgeissuerpermission(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    if (haspermission(params[0].get_str(), "mine") && isbadgecreator(params[0].get_str(), params[1].get_str()))
    {
        Array ext_params;
        Object data;

        data.push_back(Pair("data",params[2]));

        BOOST_FOREACH(const Value& value, params)
        {
            ext_params.push_back(value);
        }
        ext_params.push_back("revoke");

        return writebadgeissuerpermission(ext_params, fHelp);
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }
}

//START GRANT REVOKE CENSOR

// param0 - from_address
// param1 - key
// param2 - data
// param3 - "grant | revoke"

// param1 - badge creator
// param2 - badge transaction id found in rootbadges
// param3 - address of badge issuer
// param4 - badge issuer permission

Value writemessagecensorpermission(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    Array keyTxidParams;
    Array keyAddressParams;
    Array AddressDataArray;
    Array TxidDataArray;

    Object keyTxidData;
    Object keyAddressData;

    Object addresses;

    Object contentEntry1;

    contentEntry1.push_back(Pair("data",params[2]));
    contentEntry1.push_back(Pair("permission",params[3]));

    const Value& json_dataEntry1 = contentEntry1;
    const std::string string_dataEntry1 = write_string(json_dataEntry1, false);
    
    std::string hex_dataEntry1 = HexStr(string_dataEntry1.begin(), string_dataEntry1.end());

    keyTxidData.push_back(Pair("for", STREAM_CENSORINGRIGHTS));
    keyTxidData.push_back(Pair("key", params[1]));
    keyTxidData.push_back(Pair("data", hex_dataEntry1));

    TxidDataArray.push_back(keyTxidData);
    keyTxidParams.push_back(params[0]); // from-address
    keyTxidParams.push_back(addresses); // addresses
    keyTxidParams.push_back(TxidDataArray); // data array

    Object contentEntry2;

    contentEntry2.push_back(Pair("data",params[1]));
    contentEntry2.push_back(Pair("permission",params[3]));

    const Value& json_dataEntry2 = contentEntry2;
    const std::string string_dataEntry2 = write_string(json_dataEntry2, false);
    
    std::string hex_dataEntry2 = HexStr(string_dataEntry2.begin(), string_dataEntry2.end());

    keyAddressData.push_back(Pair("for", STREAM_CENSORINGRIGHTS));
    keyAddressData.push_back(Pair("key", params[2]));
    keyAddressData.push_back(Pair("data", hex_dataEntry2));

    AddressDataArray.push_back(keyAddressData);
    keyAddressParams.push_back(params[0]); // from-address
    keyAddressParams.push_back(addresses); // addresses
    keyAddressParams.push_back(AddressDataArray); // data array

    createrawsendfrom(keyTxidParams, fHelp);
    return createrawsendfrom(keyAddressParams, fHelp);
}

// param0 - from_address
// param1 - key
// param2 - data
Value grantmessagecensorpermission(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;

    BOOST_FOREACH(const Value& value, params)
    {
        ext_params.push_back(value);
    }
    ext_params.push_back("grant");

    return writemessagecensorpermission(ext_params, fHelp);
}

// param0 - from_address
// param1 - key
// param2 - data
Value revokemessagecensorpermission(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;

    BOOST_FOREACH(const Value& value, params)
    {
        ext_params.push_back(value);
    }
    ext_params.push_back("revoke");

    return writemessagecensorpermission(ext_params, fHelp);
}

//END GRAND REVOKE CENSOR

// param1 - badge creator
// param2 - badge transaction id found in rootbadges
// param3 - badge annotations
// param4 - type

Value writeannotatedbadge(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    if (haspermission(params[0].get_str(), "mine") && isbadgecreator(params[0].get_str(), params[1].get_str())) 
    {

        Array ext_params;
        Object content;

        content.push_back(Pair("type",params[3]));
        content.push_back(Pair("data",params[2]));

        const Value& json_data = content;
        const std::string string_data = write_string(json_data, false);

        std::string hex_data = HexStr(string_data.begin(), string_data.end());

        ext_params.push_back(params[0]); // badge creator
        ext_params.push_back(STREAM_ANNOTATEDBADGES); // stream for annotating
        ext_params.push_back(params[1]); // transaction id of annotated badge
        ext_params.push_back(hex_data);

        return publishfrom(ext_params, fHelp);
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }
}

// param1 - badge creator
// param2 - badge transaction id found in rootbadges
// param3 - badge annotations

Value annotatebadge(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;

    if (haspermission(params[0].get_str(), "mine")  && isbadgecreator(params[0].get_str(), params[1].get_str()))
    {
        BOOST_FOREACH(const Value& value, params)
        {
            ext_params.push_back(value);
        }
        ext_params.push_back("annotate");
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }

    return writeannotatedbadge(ext_params, fHelp);
}

// param1 - category creator/modifier
// param2 - category key - use "rootcategories" for parent categories, use <parent_tx_id> for child categories
// param3 - category data
// Should include the following:
// Corresponding Record Type: { record_type: <record_type_key> }

Value writecategory(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    if (haspermission(params[0].get_str(), "admin"))
    {
        Array ext_params;
        Object data;

        data.push_back(Pair("data",params[2]));

        const Value& json_data = data;
        const std::string string_data = write_string(json_data, false);

        std::string hex_data = HexStr(string_data.begin(), string_data.end());

        ext_params.push_back(params[0]); // category creator/modifier
        ext_params.push_back(STREAM_CATEGORIES); // stream for categories
        ext_params.push_back(params[1]); // category identifier
        ext_params.push_back(hex_data);

        return publishfrom(ext_params, fHelp);
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }
}

// param1 - category creator/modifier
// param2 - category txid found in STREAM_CATEGORIES
// param3 - category data
// Should include the following:
// Corresponding Record Type: { record_type: <record_type_key> }
// Fields of latest category and new values
// param4 - update type (update/delete)

Value writeupdatecategories(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    if (haspermission(params[0].get_str(), "admin"))
    {

        Array ext_params;
        Object data;

        data.push_back(Pair("update_type",params[3]));
        data.push_back(Pair("data",params[2]));

        const Value& json_data = data;
        const std::string string_data = write_string(json_data, false);

        std::string hex_data = HexStr(string_data.begin(), string_data.end());

        ext_params.push_back(params[0]); // category creator/modifier
        ext_params.push_back(STREAM_UPDATECATEGORIES); // stream for category updates/deletes
        ext_params.push_back(params[1]); // category identifier
        ext_params.push_back(hex_data);

        return publishfrom(ext_params, fHelp);
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }
}

// param1 - category creator/modifier
// param2 - category txid found in STREAM_CATEGORIES
// param3 - category data
// Should include the following:
// Corresponding Record Type: { record_type: <record_type_key> }
// Fields of latest category and new values

Value updatecategory(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;

    if (haspermission(params[0].get_str(), "admin"))
    {
        BOOST_FOREACH(const Value& value, params)
        {
            ext_params.push_back(value);
        }
        ext_params.push_back("update");
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }

    return writeupdatecategories(ext_params, fHelp);
}

// param1 - category creator/modifier
// param2 - category txid found in STREAM_CATEGORIES
// param3 - category data
// Should include the following:
// Corresponding Record Type: { record_type: <record_type_key> }
// Fields of latest category and new values

Value deletecategory(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;

    if (haspermission(params[0].get_str(), "admin"))
    {
        BOOST_FOREACH(const Value& value, params)
        {
            ext_params.push_back(value);
        }
        ext_params.push_back("delete");
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }

    return writeupdatecategories(ext_params, fHelp);
}

// param1 - category creator/modifier
// param2 - category key - use "rootcategories" for parent categories, use <parent_tx_id> for child categories
// param3 - category data
// Should include the following:
// Corresponding Record Type: { record_type: <record_type_key> }

Value writecustomcategory(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;
    Object data;

    data.push_back(Pair("data",params[2]));

    const Value& json_data = data;
    const std::string string_data = write_string(json_data, false);

    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    ext_params.push_back(params[0]); // category creator/modifier
    ext_params.push_back(STREAM_CUSTOMCATEGORIES); // stream for categories
    ext_params.push_back(params[1]); // category identifier
    ext_params.push_back(hex_data);

    return publishfrom(ext_params, fHelp);
}

// param1 - category creator/modifier
// param2 - category txid found in STREAM_CUSTOMCATEGORIES
// param3 - category data
// Should include the following:
// Corresponding Record Type: { record_type: <record_type_key> }
// Fields of latest category and new values
// param4 - update type (update/delete)

Value writeupdatecustomcategories(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    Array ext_params;
    Object data;

    data.push_back(Pair("update_type",params[3]));
    data.push_back(Pair("data",params[2]));

    const Value& json_data = data;
    const std::string string_data = write_string(json_data, false);

    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    ext_params.push_back(params[0]); // category creator/modifier
    ext_params.push_back(STREAM_UPDATECUSTOMCATEGORIES); // stream for category updates/deletes
    ext_params.push_back(params[1]); // category identifier
    ext_params.push_back(hex_data);

    return publishfrom(ext_params, fHelp);
}

// param1 - category creator/modifier
// param2 - category txid found in STREAM_CUSTOMCATEGORIES
// param3 - category data
// Should include the following:
// Corresponding Record Type: { record_type: <record_type_key> }
// Fields of latest category and new values

Value updatecustomcategory(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;

    BOOST_FOREACH(const Value& value, params)
    {
        ext_params.push_back(value);
    }
    ext_params.push_back("update");

    return writeupdatecustomcategories(ext_params, fHelp);
}

// param1 - category creator/modifier
// param2 - category txid found in STREAM_CUSTOMCATEGORIES
// param3 - category data
// Should include the following:
// Corresponding Record Type: { record_type: <record_type_key> }
// Fields of latest category and new values

Value deletecustomcategory(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;

    BOOST_FOREACH(const Value& value, params)
    {
        ext_params.push_back(value);
    }
    ext_params.push_back("delete");

    return writeupdatecustomcategories(ext_params, fHelp);
}

// param1 - record type creator/modifier
// param2 - record type key
// param3 - record type data
// Should include the following:
// Corresponding Category: { category: <category_key> }

Value writerecordtype(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    if (haspermission(params[0].get_str(), "admin"))
    {
        Array ext_params;
        Object data;

        data.push_back(Pair("data",params[2]));

        const Value& json_data = data;
        const std::string string_data = write_string(json_data, false);

        std::string hex_data = HexStr(string_data.begin(), string_data.end());

        ext_params.push_back(params[0]); // record type creator/modifier
        ext_params.push_back(STREAM_RECORDTYPES); // stream for record types
        ext_params.push_back(params[1]); // record type identifier
        ext_params.push_back(hex_data);

        return publishfrom(ext_params, fHelp);
    }
    else
    {
        throw runtime_error("Unauthorized address\n");
    }
}

bool doesservicexist(std::string txid)
{
    try {
        Array service_params;
        service_params.push_back(STREAM_SERVICES);
        service_params.push_back(txid);
        Array service_results = getstreamitem(service_params, false).get_array();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// param1 - from-address
// param2 - JSON of service details
// param3 - (optional) asset name of service
// param4 - (optional) quantity of service
Value listservice(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2)
        throw runtime_error("Help message not found\n");


    std::string service_name;

    BOOST_FOREACH(const Pair& pair, params[1].get_obj())
    {
        if (pair.name_ == "productname")
        {
            service_name = pair.value_.get_str();
            break;
        }
    }

    Object data;
    data.push_back(Pair("name", service_name));
    data.push_back(Pair("data", params[1]));

    Array ext_params;

    Object addresses;
    Array dataArray;
    
    if (params.size() > 2)
    {
        if (doesassetexist(params[2].get_str()))
            throw runtime_error("Service already exists.");

        Object issue_params;
        Object issue_raw;

        int quantity = atoi(params[3].get_str().c_str());

        issue_raw.push_back(Pair("raw", quantity));
        const Value& value_issue_raw = issue_raw;
        issue_params.push_back(Pair("issue", value_issue_raw));
        const Value& value_issue_params = issue_params;

        if (haspermission(params[0].get_str(), "mine"))
        {
            data.push_back(Pair("asset-holder", params[0]));
            addresses.push_back(Pair(params[0].get_str(), value_issue_params));    
        }
        else
        {
            Array multisig_params;
            multisig_params.push_back("3");
            std::string multisig = getauthmultisigaddress(multisig_params, false).get_str();
            addresses.push_back(Pair(multisig, value_issue_params));

            data.push_back(Pair("asset-holder", multisig));
        }

        Object asset_data;
        Object asset_metadata;
        asset_metadata.push_back(Pair("owner", params[0].get_str()));
        const Value& value_asset_metadata = asset_metadata;

        asset_data.push_back(Pair("create", "asset"));
        asset_data.push_back(Pair("name", params[2].get_str()));
        asset_data.push_back(Pair("open", true));
        asset_data.push_back(Pair("details", value_asset_metadata));

        dataArray.push_back(asset_data);
    }

    const Value& json_data = data;
    const std::string string_data = write_string(json_data, false);

    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_SERVICES));
    raw_data.push_back(Pair("key", "service"));
    raw_data.push_back(Pair("data", hex_data));

    dataArray.push_back(raw_data);

    ext_params.push_back(params[0]); // from-address
    ext_params.push_back(addresses); // addresses
    ext_params.push_back(dataArray); // data array

    return createrawsendfrom(ext_params, fHelp);
}

// param1 - from-address
// param2 - stream txid
// param3 - JSON of service details
// param4 - type
Value writeannotatedservice(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    Array pub_params;
    pub_params.push_back(STREAM_SERVICES);
    pub_params.push_back(params[1]);
    Object result = getstreamitem(pub_params, false).get_obj();

    if (result.size() > 0) {
        std::string publisher_item = result[0].value_.get_array().back().get_str();
            if (strcmp(publisher_item.c_str(), params[0].get_str().c_str()) != 0) {
                throw runtime_error("Address is not the previous publisher");
        }
    }

    Object data;

    data.push_back(Pair("type",params[3]));
    data.push_back(Pair("data",params[2]));

    const Value& json_data = data;
    const std::string string_data = write_string(json_data, false);

    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_SERVICES));
    raw_data.push_back(Pair("key", params[1]));
    raw_data.push_back(Pair("data", hex_data));

    Array ext_params;

    Object addresses;
    Array dataArray;
    dataArray.push_back(raw_data);
    ext_params.push_back(params[0]); // from-address
    ext_params.push_back(addresses); // addresses
    ext_params.push_back(dataArray); // data array

    return createrawsendfrom(ext_params, fHelp);
}

// param1 - from-address
// param2 - stream txid
// param3 - data
Value updateservice(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;

    BOOST_FOREACH(const Value& value, params)
    {
        ext_params.push_back(value);
    }
    ext_params.push_back("update");

    return writeannotatedservice(ext_params, fHelp);
}

// param1 - from-address
// param2 - stream txid
// param3 - data
Value delistservice(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;

    BOOST_FOREACH(const Value& value, params)
    {
        ext_params.push_back(value);
    }
    ext_params.push_back("delist");

    return writeannotatedservice(ext_params, fHelp);
}

Value hexStrToJson(Value v_str) {
    std::vector<unsigned char> vucJson(ParseHex(v_str.get_str().c_str()));
    std::stringstream ss;
    for(unsigned int i=0;i<vucJson.size();++i) {
        ss << vucJson[i];
    }
    Value val;
    read_string(ss.str(), val);
    return val;
}

// param1 - service txid
Value getservice(const Array& params, bool fHelp)
{
    Array service_params;
    service_params.push_back(STREAM_SERVICES);
    service_params.push_back(params[0]);
    Object service_result = getstreamitem(service_params, fHelp).get_obj();    

    BOOST_FOREACH(const Pair& d, service_result) 
    {
        if (d.name_ == "data") {
            if (d.value_.type() == obj_type) {
                int vout = 0;
                BOOST_FOREACH(const Pair& subpair, d.value_.get_obj()) 
                {
                    if (subpair.name_ == "vout") {
                        vout = subpair.value_.get_int();
                    }
                }
                Array txout_params;
                txout_params.push_back(params[0]); // the txid should be the same as the stream item itself
                txout_params.push_back(vout);
                Value v = gettxoutdata(txout_params, fHelp);
                return hexStrToJson(v);
            }
            else {
                // only other option is string
                return hexStrToJson(d.value_);
            }
        }
    }  

    return getstreamitem(service_params, false).get_obj();    
}

// get Value in service data by name
Value getservicedata(Value service_id, std::string name)
{
    Array service_params;
    service_params.push_back(service_id);
    Object service_result = getservice(service_params, false).get_obj();
    BOOST_FOREACH(const Pair& d, service_result) 
    {
        if (d.name_ == "data") {
            BOOST_FOREACH(const Pair& subpair, d.value_.get_obj()) 
            {
                if (subpair.name_ == name) {
                    return subpair.value_;
                }
            }
        }
    }  
    return "";
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

// param1 - from-address (buyer's address)
// param2 - service txid
// param3 - name of service
// param4 - total amount
// param5 - badge notes, encrypted for badge creator (optional)
// param6 - badge notes, encrypted for seller (optional)
// param7 - user-defined quantity (optional)
// param8 - conversion rate
Value purchasenonconsumableservice(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 4)
        throw runtime_error("Help message not found\n");

    // support either string or number for param4
    double amount = 0.0;
    if (params[3].type() == str_type) 
    {
        amount = atof(params[3].get_str().c_str());
    }
    if (params[3].type() == real_type) 
    {
        amount = params[3].get_real();
    }

    // support either string or number for param7
    int qty = 1;
    if (params.size() > 6)
    {
        if (params[6].type() == str_type)
        {
            qty = atoi(params[6].get_str().c_str());
        }
        if (params[6].type() == int_type)
        {
            qty = params[6].get_int();
        }
    }

    // support either string or number for param8
    double conversion_rate = 1.0;
    if (params[7].type() == str_type)
    {
        conversion_rate = atof(params[7].get_str().c_str());
    }
    if (params[7].type() == real_type)
    {
        conversion_rate = params[7].get_real();
    }

    Array service_params;
    service_params.push_back(STREAM_SERVICES);
    service_params.push_back(params[1]);
    Object service_result = getstreamitem(service_params, false).get_obj();

    std::string publisher = service_result[0].value_.get_array().back().get_str();

    std::string funds_receiver = publisher;

    Object final_purchase_data;
    Object purchase_data;
    purchase_data.push_back(Pair("servicetxid", params[1]));
    purchase_data.push_back(Pair("servicename", params[2]));

    purchase_data.push_back(Pair("selleraddress", publisher));
    purchase_data.push_back(Pair("buyeraddress", params[0]));
    purchase_data.push_back(Pair("amount", amount));
    purchase_data.push_back(Pair("quantity", "0")); // identifier in purchase completion if service is a nonconsumable service
    purchase_data.push_back(Pair("userdefined_quantity", qty)); // record the number of instances purchased
    purchase_data.push_back(Pair("conversion_rate", conversion_rate)); // record conversion rate at the time of purchase
    if (params.size() > 4) {
        purchase_data.push_back(Pair("badgenotescreator", params[4]));
    }
    if (params.size() > 5) {
        purchase_data.push_back(Pair("badgenotesseller", params[5]));
    }

    // automatically derive escrow status and escrow address
    Value exp_period = getservicedata(params[1], "expirationperiod");
    bool is_escrow = is_number(exp_period.get_str()) && strcmp(exp_period.get_str().c_str(), "0") != 0;
    if (is_escrow)
    {
        Array escrow_params;
        escrow_params.push_back("3"); // TODO: Constant
        escrow_params.push_back(params[0]);
        std::string escrow_address = getescrowmultisigaddress(escrow_params, false).get_str();
        funds_receiver = escrow_address;
        purchase_data.push_back(Pair("multisigaddress", escrow_address));
        purchase_data.push_back(Pair("status", "in escrow"));
    }
    else
    {
        purchase_data.push_back(Pair("multisigaddress", "none"));
        purchase_data.push_back(Pair("status", "completed"));
    }

    Object addresses;
    Array data_array;
    Array ext_params;

    addresses.push_back(Pair(funds_receiver, amount));

    purchase_data.push_back(Pair("toaddress", funds_receiver));
    final_purchase_data.push_back(Pair("data", purchase_data));

    const Value& json_data = final_purchase_data;
    const std::string string_data = write_string(json_data, false);

    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_PURCHASESTATUS));
    raw_data.push_back(Pair("key", "rootpurchases"));
    raw_data.push_back(Pair("data", hex_data));

    data_array.push_back(raw_data);

    ext_params.push_back(params[0]);
    ext_params.push_back(addresses);
    ext_params.push_back(data_array);

    return createrawsendfrom(ext_params, fHelp);
}


// param1 - from-address (buyer's address)
// param2 - service txid
// param3 - name of service
// param4 - total amount
// param5 - quantity
// param6 - escrow address (optional)
// param7 - badge notes, encrypted for badge creator (optional)
// param8 - badge notes, encrypted for seller (optional)
// param9 - conversion rate
Value purchaseconsumableservice(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 5)
        throw runtime_error("Help message not found\n");

    bool is_escrow = false;

    if (params.size() > 5 && params[5].get_str().length() > 0)
    {
        is_escrow = true;
    }

    Array service_params;
    service_params.push_back(STREAM_SERVICES);
    service_params.push_back(params[1]);
    Object service_result = getstreamitem(service_params, false).get_obj();

    std::string publisher = service_result[0].value_.get_array().back().get_str();

    Array getinfo_params;
    Object info = getinfo(getinfo_params,false).get_obj();

    std::string burn_address = info[9].value_.get_str();

    std::string funds_receiver = publisher;
    std::string assets_receiver = burn_address;

     // support either string or number for param9
    double conversion_rate = 1.0;
    if (params[8].type() == str_type)
    {
        conversion_rate = atof(params[8].get_str().c_str());
    }
    if (params[8].type() == real_type)
    {
        conversion_rate = params[8].get_real();
    }

    Object final_purchase_data;
    Object purchase_data;
    purchase_data.push_back(Pair("servicetxid", params[1]));
    purchase_data.push_back(Pair("servicename", params[2]));

    purchase_data.push_back(Pair("selleraddress", publisher));
    purchase_data.push_back(Pair("buyeraddress", params[0]));
    purchase_data.push_back(Pair("amount", params[3]));
    purchase_data.push_back(Pair("quantity", params[4]));
    purchase_data.push_back(Pair("conversion_rate", conversion_rate)); // record conversion rate at the time of purchase
    if (params.size() > 6) {
        purchase_data.push_back(Pair("badgenotescreator", params[6]));
    }
    if (params.size() > 7) {
        purchase_data.push_back(Pair("badgenotesseller", params[7]));
    }

    if (is_escrow)
    {
        std::string escrow_address = params[5].get_str();
        funds_receiver = escrow_address;
        assets_receiver = escrow_address;
        purchase_data.push_back(Pair("multisigaddress", escrow_address));
        purchase_data.push_back(Pair("status", "in escrow"));
    }
    else
    {
        purchase_data.push_back(Pair("multisigaddress", "none"));
        purchase_data.push_back(Pair("status", "completed"));
    }

    Object first_addresses;
    Array first_data_array;
    Array first_ext_params;

    purchase_data.push_back(Pair("toaddress", funds_receiver));
    final_purchase_data.push_back(Pair("data", purchase_data));

    const Value& first_json_data = final_purchase_data;
    const std::string first_string_data = write_string(first_json_data, false);

    std::string first_hex_data = HexStr(first_string_data.begin(), first_string_data.end());

    Object first_raw_data;
    first_raw_data.push_back(Pair("for", STREAM_PURCHASESTATUS));
    first_raw_data.push_back(Pair("key", "rootpurchases"));
    first_raw_data.push_back(Pair("data", first_hex_data));

    first_data_array.push_back(first_raw_data);

    first_ext_params.push_back(params[0]);
    first_ext_params.push_back(first_addresses);
    first_ext_params.push_back(first_data_array);

    return createrawsendfrom(first_ext_params, fHelp).get_str();
}

// param1 - from-address
// param2 - Transaction id of activity to be logged
// param3 - Stream 
Value logactivity(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error("Help message not found\n");

    Object data;
    data.push_back(Pair("txid", params[1]));
    data.push_back(Pair("stream", params[2]));

    const Value& json_data = data;
    const std::string string_data = write_string(json_data, false);

    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    Array ext_params;

    ext_params.push_back(params[0]); // from-address
    ext_params.push_back(STREAM_ACTIVITIES); // stream
    ext_params.push_back(params[0]); //stream of txid
    ext_params.push_back(hex_data); // data hex

    return publishfrom(ext_params, fHelp);
}

// param1 - from-address
// param2 - to-address
// param3 - payload
// param4 - expiry date
// param5 - keys json
// return rawtx
Value sharetxn(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() == 4 )
        throw runtime_error("Help message not found\n");

    Object data;

    data.push_back(Pair("from_address",params[0]));
    data.push_back(Pair("to_address",params[1]));
    data.push_back(Pair("payload",params[2]));
    if (params.size() > 3) {
        data.push_back(Pair("expiry_date",params[3]));
        data.push_back(Pair("keys",params[4]));
    }

    const Value& json_data = data;
    const std::string string_data = write_string(json_data, false);

    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_SHAREDTXNS));
    raw_data.push_back(Pair("key", params[1])); // to_address
    raw_data.push_back(Pair("data", hex_data));

    Array ext_params;

    Object addresses;
    Array dataArray;
    dataArray.push_back(raw_data);
    ext_params.push_back(params[0]); // from-address
    ext_params.push_back(addresses); // addresses
    ext_params.push_back(dataArray); // data array

    return createrawsendfrom(ext_params, fHelp);
}

// param1 - from-address
// param2 - to-address (asset holder)
// param3 - Asset Issue TXID
// param4 - Quantity to add from the service
Value addservicequantity(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    Array pub_params;
    pub_params.push_back(STREAM_SERVICES);
    pub_params.push_back(params[2]);
    Object result = getstreamitem(pub_params, false).get_obj();
  
    if (result.size() > 0) {
        std::string publisher_item = result[0].value_.get_array().back().get_str();
            if (strcmp(publisher_item.c_str(), params[0].get_str().c_str()) != 0) {
                throw runtime_error("Address is not the previous publisher");
        }
    }

    Object address;
    Object address_data;
    Object issuemore_data;

    issuemore_data.push_back(Pair("asset", params[2]));
    issuemore_data.push_back(Pair("raw", atoi(params[3].get_str().c_str())));
    address_data.push_back(Pair("issuemore",issuemore_data));
    address.push_back(Pair(params[1].get_str(),address_data)); // issue the asset to the asset holder
 
    Array ext_params;
    ext_params.push_back(params[0]); // from-address
    ext_params.push_back(address); // address and issuemore data
  
    return createrawsendfrom(ext_params, fHelp);
    
}

// param1 - service creator
// param2 - asset holder
// param2 - Asset Issue TXID
// param3 - Quantity to remove from the service
Value removeservicequantity(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error("Help message not found\n");

    Array pub_params;
    pub_params.push_back(STREAM_SERVICES);
    pub_params.push_back(params[2]);
    Object result = getstreamitem(pub_params, false).get_obj();

    if (result.size() > 0) {
        std::string publisher_item = result[0].value_.get_array().back().get_str();
            if (strcmp(publisher_item.c_str(), params[0].get_str().c_str()) != 0) {
                throw runtime_error("Address is not the previous publisher");
        }
    }
    Array getinfo_params;
    Object info = getinfo(getinfo_params,false).get_obj();
    
    Object address;
    Object address_data;
    address_data.push_back(Pair(params[2].get_str(),atoi(params[3].get_str().c_str()))); //asset issuetxid, amount to burn
    address.push_back(Pair(info[9].value_.get_str(),address_data));

    Array ext_params;
    ext_params.push_back(params[1]); // from-address
    ext_params.push_back(address); // address and issuemore data
    std::string rawtx = createrawsendfrom(ext_params, fHelp).get_str();

    Array ext_params2;
    ext_params2.push_back(rawtx); // rawtx
    ext_params2.push_back(params[0]); // triggeraddress
    return appendrawsendfrom(ext_params2, fHelp);
    
}

// param0 - from-address
// param1 - ROOT TXID 
// param2 - JSON details
// param3 - 'true' to sign the rawtransaction, from-address must be a multisig address
Value updatepurchasestatus(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3)
        throw runtime_error("Help message not found\n");

    Object content;
    content.push_back(Pair("data",params[2]));

    const Value& json_data = content;
    const std::string string_data = write_string(json_data, false);
    
    std::string hex_data = HexStr(string_data.begin(), string_data.end());


    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_PURCHASESTATUS));
    raw_data.push_back(Pair("key", params[1])); // Root TXID
    raw_data.push_back(Pair("data", hex_data));

    Array ext_params;

    Object addresses;
    Array dataArray;
    dataArray.push_back(raw_data);
    ext_params.push_back(params[0]); // from-address
    ext_params.push_back(addresses); // addresses
    ext_params.push_back(dataArray); // data array

    if(params.size() == 4  && strcmp(params[3].get_str().c_str(),"true"))
        ext_params.push_back("sign"); // sign the rawtx
    
     
    return createrawsendfrom(ext_params, fHelp);
}

// param0 - escrowaddress
// param1 - ROOT TXID  
// param2 - Quantity of asset
// param3 - vendor-address
// param4 - Amount to pay vendor
// param5 - buyeraddress
// param6 - JSON Details
// param7 - triggeraddress
// param8 - Asset Issue TXID 
Value completepurchase(const Array& params, bool fHelp)
{   
    if (fHelp || params.size() != 9)
        throw runtime_error("Help message not found\n");

    // Get info for burnaddress
    Array getinfo_params;
    Object info = getinfo(getinfo_params,false).get_obj();

    Object addresses;
    
    Object burn_data;
    // Transfer 

    // asset escrow -> burnaddress
    // pushback(asset id, quantity)
    if (atoi(params[2].get_str().c_str()) != 0 ){
        burn_data.push_back(Pair(params[8].get_str(),atoi(params[2].get_str().c_str()))); 
        addresses.push_back(Pair(info[9].value_.get_str(),burn_data)); //burnaddress
    }
    
    // money escrow -> vendor 
    // pushback (vendor address, amount to pay vendor)
    addresses.push_back(Pair(params[3].get_str(),atof(params[4].get_str().c_str())));

    // Convert data to Hex
    Object content;
    content.push_back(Pair("data",params[6]));
    const Value& json_data = content;
    const std::string string_data = write_string(json_data, false);
    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    // Publish to Stream
    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_PURCHASESTATUS));
    raw_data.push_back(Pair("key", params[1])); // Root TXID
    raw_data.push_back(Pair("data", hex_data));
    Array dataArray;
    dataArray.push_back(raw_data);
    
    Array ext_params;
    ext_params.push_back(params[0]); // escrowaddress
    ext_params.push_back(addresses); // address and issuemore data
    ext_params.push_back(dataArray); // data array
    std::string rawtx = createrawsendfrom(ext_params, fHelp).get_str();

    Array ext_params2;
    ext_params2.push_back(rawtx); // rawtx
    ext_params2.push_back(params[7]); // triggeraddress
    return appendrawsendfrom(ext_params2, fHelp);
    
}

// param0 - escrowaddress
// param1 - root txid
// param2 - Quantity of asset
// param3 - vendor-address
// param4 - Amount to pay vendor
// param5 - buyer-address
// param6 - Amount to pay buyer
// param7 - Json Details
// param8 - triggeraddress
// param9 - Asset Issue TXID 
// param10 - asset holder
Value refundpurchase(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 11)
        throw runtime_error("Help message not found\n");
    
    Object addresses;
    Object vendor_data;

    // asset escrow -> vendor
    // pushback(asset id, quantity)
    if (atoi(params[2].get_str().c_str()) != 0 ){
        vendor_data.push_back(Pair(params[9].get_str(),atoi(params[2].get_str().c_str()))); //asset issuetxid, amount to burn 
        addresses.push_back(Pair(params[10].get_str(),vendor_data));
    }
       

    // money escrow -> vendor 
    // pushback (vendor address, amount to pay vendor)
    addresses.push_back(Pair(params[3].get_str(),atof(params[4].get_str().c_str())));

    // money escrow -> buyer 
    // pushback (buyer address, amount to pay buyer)
    addresses.push_back(Pair(params[5].get_str(),atof(params[6].get_str().c_str())));


    // Convert data to Hex
    Object content;
    content.push_back(Pair("data",params[7]));
    const Value& json_data = content;
    const std::string string_data = write_string(json_data, false);
    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    // Publish to Stream
    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_PURCHASESTATUS));
    raw_data.push_back(Pair("key", params[1])); // ROOT TXID
    raw_data.push_back(Pair("data", hex_data));
    Array dataArray;
    dataArray.push_back(raw_data);
    
    Array ext_params;
    ext_params.push_back(params[0]); // escrowaddress
    ext_params.push_back(addresses); // address and issuemore data
    ext_params.push_back(dataArray); // data array
    std::string rawtx = createrawsendfrom(ext_params, fHelp).get_str();

    Array ext_params2;
    ext_params2.push_back(rawtx); // rawtx
    ext_params2.push_back(params[8]); // triggeraddress
    return appendrawsendfrom(ext_params2, fHelp);

}

// param0 - rawtx
// param1 - from-address
Value appendrawsendfrom(const Array& params, bool fHelp)
{   
    if (fHelp || params.size() < 2 || params.size() > 5) 
        throw runtime_error("Help message not found\n");    

    Array addressesArray;
    addressesArray.push_back(params[1]);

    Array listunspent_params;
    listunspent_params.push_back(1); // min confirmations
    listunspent_params.push_back(9999999); // max confirmations
    listunspent_params.push_back(addressesArray); // addresses array
    Array unspentItems = listunspent(listunspent_params, fHelp).get_array();

    std::string rawtx = params[0].get_str();
    // we estimate that this function will add around 500 bytes to the TX (generous allowance)
    unsigned int nSize = rawtx.length() + 500;
    char msg[3000];

    // estimate the needed tx fee
    unsigned int minRelayTxFee = StreamUtils::GetMinimumRelayTxFee();
    unsigned int estFee = minRelayTxFee*nSize / 1000;
    std::string address = params[1].get_str();
    if (haspermission(address, "mine") || multisighaspermission(address, "mine")) 
    {
        // miners dont need no fee
        estFee = 0;
    }

    Array accumulatedTransactionsArr;
    unsigned int accumulatedAmount = 0;
    
    // look for an unspent item that covers the needed fee
    BOOST_FOREACH(const Value& data, unspentItems)
    {
        Object results = data.get_obj();
        std::string txid;
        int vout = 0;
        bool amountFound = false;
        bool smallerAmountFound = false;
        BOOST_FOREACH(const Pair& d, results) 
        {
            if (d.name_ == "txid") 
            {
                txid = d.value_.get_str();
            }
            if (d.name_ == "vout") 
            {
                vout = d.value_.get_int();
            }
            if (d.name_ == "amount") 
            {
                // convert amount to satoshis
                unsigned int satoshis = d.value_.get_real()*10000000; // multiplier is fixed for amberchain
                if (satoshis >= estFee) 
                {
                    amountFound = true;
                } 
                else 
                {
                    // accumulate smaller amounts if possible
                    if (accumulatedAmount < estFee) 
                    {
                        accumulatedAmount += satoshis;
                        smallerAmountFound = true;
                    }
                }
            }
        }        
        if (smallerAmountFound) 
        {
            sprintf(msg+strlen(msg),"\naccumulating an input!: %s.",txid.c_str());
            Object transactionObj;
            transactionObj.push_back(Pair("txid", txid));
            transactionObj.push_back(Pair("vout", vout));
            accumulatedTransactionsArr.push_back(transactionObj);
        }
        if (amountFound) 
        {
            // build a raw transaction from the chosen txid and vout
            Array transactionsArr;
            Object transactionObj;
            transactionObj.push_back(Pair("txid", txid));
            transactionObj.push_back(Pair("vout", vout));
            transactionsArr.push_back(transactionObj);
            
            Array ext_params;
            ext_params.push_back(rawtx); // rawtx
            ext_params.push_back(transactionsArr); // UTXOs
            std::string appendedrawtx = appendrawtransaction(ext_params, fHelp).get_str();

            Array ext_params2;
            ext_params2.push_back(appendedrawtx); // rawtx
            ext_params2.push_back(params[1]); // change address
            return appendrawchange(ext_params2, fHelp);
        }
    }    

    // we exited the loop without finding a single input that satisfies the est fee
    // let's see if we have accumulated smaller inputs instead
    sprintf(msg+strlen(msg),"\naccumulatedAmount: %d.",accumulatedAmount);
    if (accumulatedAmount >= estFee)
    {
        Array ext_params;
        ext_params.push_back(rawtx); // rawtx
        ext_params.push_back(accumulatedTransactionsArr); // UTXOs
        std::string appendedrawtx = appendrawtransaction(ext_params, fHelp).get_str();

        Array ext_params2;
        ext_params2.push_back(appendedrawtx); // rawtx
        ext_params2.push_back(params[1]); // change address
        return appendrawchange(ext_params2, fHelp);
    }

    sprintf(msg+strlen(msg),"\nInsufficient funds (appendrawsendfrom): %d.",estFee);
    LogPrintf(msg);
    
    // we can't find any valid UTXOs to spend, return error
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Insufficient funds.");
}

// param0 - escrowaddress
// param1 - Root TXID 
// param2 - Quantity of asset
// param3 - vendor-address
// param4 - Amount to pay vendor
// param5 - buyer-address
// param6 - Amount to pay buyer
// param7 - Json Details
// param8 - Asset Issue TXID 
// param9 - asset holder
Value expirepurchase(const Array& params, bool fHelp)
{   
    if (fHelp || params.size() != 10)
        throw runtime_error("Help message not found\n");

    // Get info for burnaddress
    Array getinfo_params;
    Object info = getinfo(getinfo_params,false).get_obj();
   
    Object addresses;

    Object burn_data;
    // Transfer 
    // asset escrow -> burnaddress
    // pushback(asset id, quantity)
    if (atoi(params[2].get_str().c_str()) != 0 ){
        burn_data.push_back(Pair(params[8].get_str(),atoi(params[2].get_str().c_str()))); //asset issuetxid, amount to burn 
        addresses.push_back(Pair(params[9].get_str(),burn_data));
    }
   
    // Transfer 
    // money escrow -> vendor 
    // pushback (vendor address, amount to pay vendor)
    addresses.push_back(Pair(params[3].get_str(),atof(params[4].get_str().c_str())));

    // money escrow -> buyer 
    // pushback (buyer address, amount to pay buyer)
    addresses.push_back(Pair(params[5].get_str(),atof(params[6].get_str().c_str())));


    // Convert data to Hex
    Object content;
    content.push_back(Pair("data",params[7]));
    const Value& json_data = content;
    const std::string string_data = write_string(json_data, false);
    std::string hex_data = HexStr(string_data.begin(), string_data.end());


    // Publish to Stream
    Object raw_data;
    raw_data.push_back(Pair("for", STREAM_PURCHASESTATUS));
    raw_data.push_back(Pair("key", params[1])); // ROOT TXID
    raw_data.push_back(Pair("data", hex_data));
    Array dataArray;
    dataArray.push_back(raw_data);
    
    Array ext_params;
    ext_params.push_back(params[0]); // escrowaddress
    ext_params.push_back(addresses); // address and issuemore data
    ext_params.push_back(dataArray); // data array
    return createrawsendfrom(ext_params, fHelp);
    
}

// param1 - multisigaddress
// param2 - data (pubkeys of signatories and sigsrequired)
Value writemultisigdetails(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2)
        throw runtime_error("Help message not found\n");

    Array ext_params;
    Object data = params[1].get_obj();

    const Value& json_data = data;
    const std::string string_data = write_string(json_data, false);

    std::string hex_data = HexStr(string_data.begin(), string_data.end());

    ext_params.push_back(STREAM_MULTISIGS); // stream for multisig details
    ext_params.push_back(params[0]); // multisigaddress as key
    ext_params.push_back(hex_data);

    return publish(ext_params, fHelp);

}
/* AMB END */