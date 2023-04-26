#include "callbackHandler.h"

/****************************************************************
*
****************************************************************/
cCallbackHandler::cCallbackHandler( )
{
//		printf("   --> create cCallbackHandler::cCallbackHandler( )\n") ;
}

/****************************************************************
*
****************************************************************/
cCallbackHandler::~cCallbackHandler( )
{
//		printf("   --> cCallbackHandler::~cCallbackHandler( )\n") ;
}

/****************************************************************
*
****************************************************************/
void cCallbackHandler::unRegisterCallback(cCallbackHandler::intCallbackID cb_id)
{
//		printf("cCallbackHandler::unRegisterCallback( 000 %d ), vmEntries size: %ld\n", cb_id, vmEntries.size( )) ;
		if ( vmEntries.at(cb_id)->pCallback != nullptr)
		{
			delete vmEntries.at(cb_id)->pCallback ;
			vmEntries.at(cb_id)->pCallback = nullptr ;
			vmEntries.at(cb_id)->is_deleted = true ;
		}
}

/****************************************************************
*
****************************************************************/
void cCallbackHandler::triggerCallback(void* callback_data, bool on_error_skip)
{
//	int* data ;
//		printf("cCallbackHandler::triggerCallback( vmEntries.size(): %ld )\n", vmEntries.size()) ;
		for(int id = 0; id < vmEntries.size(); id++)
		{
			sEntry* entry = vmEntries.at(id) ;
//			data = (int *) entry->pUserData ;
//			printf("cCallbackHandler::triggerCallback( %d: %d )\n", id, *((int *) entry->pUserData )) ;
			entry->pCallback->execute(callback_data, entry->pUserData) ;
		}
}
