#pragma once

#include <iostream>
#include <vector>
struct sCBHandlerData
{
	int								mOwnerId ;
	int     					mCMDId ;
	void*   					pData ;
  std::string    		mMsg ;
} ;

/****************************************************************
*
****************************************************************/
class cGenericCallback
{
	public:
		cGenericCallback( ) 					{ }
		virtual ~cGenericCallback( ) 	{ }
		virtual void 		execute(void* callback_data, void* user_data) = 0 ;
		virtual cGenericCallback* Clone() const = 0;
};

/****************************************************************
*
****************************************************************/
class cCallbackHandler
{
	public:
		//!callback ID
		typedef int 	intCallbackID ;

		cCallbackHandler( ) ;
		~cCallbackHandler( ) ;
//---------------------------------------------------
		void 		unRegisterCallback(cCallbackHandler::intCallbackID id) ;

		template <typename T_object>
		cCallbackHandler::intCallbackID registerCallback(T_object* object, void(T_object::*function)(void*,void*), void* user_data)
		{
			//!Check empty entry and insert the callback there
			intCallbackID entry_id ;
			sEntry* 		current_entry = NULL;
			for(int id = 0; id < (int) vmEntries.size(); id++)
			{
				if((vmEntries[id])->is_deleted)
				{
					entry_id = id ;
					current_entry = vmEntries.at(id) ;
					break ;
				}
			}
			//!Add new Element?
			if(current_entry == NULL)
			{
				current_entry = new sEntry( ) ;
				vmEntries.push_back(current_entry) ;
				entry_id = vmEntries.size( ) - 1 ;
			}
			//!set values
			(vmEntries.at(entry_id))->is_deleted = false;
			(vmEntries.at(entry_id))->pCallback = new InstanceCallback<T_object>(object, function);
			(vmEntries.at(entry_id))->pUserData = user_data ;
			return entry_id ;
		}
		void triggerCallback(void* callback_data, bool on_error_skip = true) ;
//----------------------------------------------------------------
	private:
		template<class tInstClass>
		class InstanceCallback: public cGenericCallback
		{
			public:
				typedef void (tInstClass::*voidMethod)(void*, void*) ;

				//!constructor
				InstanceCallback(tInstClass* instance, voidMethod method)
				{
//					printf("InstanceCallback::InstanceCallback( )\n") ;
					pClassInstance = instance ;
					fmMethod = method ;
				}

				void execute(void* callback_data, void* user_data)
				{
					(pClassInstance->*fmMethod)(callback_data, user_data);
				}

				cGenericCallback* Clone() const
				{
					return new InstanceCallback<tInstClass>(pClassInstance, fmMethod);
				}

			private:
				tInstClass* pClassInstance ;
				voidMethod 	fmMethod ;
		};

		struct sEntry
		{
			bool 							is_deleted ;
			cGenericCallback* pCallback = nullptr ;
			void* 						pUserData ;
		} ;
		std::vector<sEntry*> vmEntries ;
} ;
