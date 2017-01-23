

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Mon Jan 23 11:10:41 2017
 */
/* Compiler settings for NScriptHost.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.00.0603 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __NScriptHost_h__
#define __NScriptHost_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __INScript_FWD_DEFINED__
#define __INScript_FWD_DEFINED__
typedef interface INScript INScript;

#endif 	/* __INScript_FWD_DEFINED__ */


#ifndef __IObject_FWD_DEFINED__
#define __IObject_FWD_DEFINED__
typedef interface IObject IObject;

#endif 	/* __IObject_FWD_DEFINED__ */


#ifndef __Parser_FWD_DEFINED__
#define __Parser_FWD_DEFINED__

#ifdef __cplusplus
typedef class Parser Parser;
#else
typedef struct Parser Parser;
#endif /* __cplusplus */

#endif 	/* __Parser_FWD_DEFINED__ */


/* header files for imported files */
#include "docobj.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __INScript_INTERFACE_DEFINED__
#define __INScript_INTERFACE_DEFINED__

/* interface INScript */
/* [uuid][dual][object] */ 


EXTERN_C const IID IID_INScript;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4C0DBEF9-DF57-455a-B119-10E4D7744E55")
    INScript : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddObject( 
            /* [in] */ BSTR name,
            /* [in] */ VARIANT object) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Exec( 
            /* [in] */ BSTR script,
            /* [retval][out] */ VARIANT *pret) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct INScriptVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INScript * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INScript * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INScript * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            INScript * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            INScript * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            INScript * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            INScript * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AddObject )( 
            INScript * This,
            /* [in] */ BSTR name,
            /* [in] */ VARIANT object);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Exec )( 
            INScript * This,
            /* [in] */ BSTR script,
            /* [retval][out] */ VARIANT *pret);
        
        END_INTERFACE
    } INScriptVtbl;

    interface INScript
    {
        CONST_VTBL struct INScriptVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INScript_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define INScript_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define INScript_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define INScript_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define INScript_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define INScript_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define INScript_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define INScript_AddObject(This,name,object)	\
    ( (This)->lpVtbl -> AddObject(This,name,object) ) 

#define INScript_Exec(This,script,pret)	\
    ( (This)->lpVtbl -> Exec(This,script,pret) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __INScript_INTERFACE_DEFINED__ */


#ifndef __IObject_INTERFACE_DEFINED__
#define __IObject_INTERFACE_DEFINED__

/* interface IObject */
/* [uuid][object] */ 


EXTERN_C const IID IID_IObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BF4F1403-B407-42aa-B642-F57B33FED8A1")
    IObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE New( 
            /* [out] */ VARIANT *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [out] */ VARIANT *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ const VARIANT *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Call( 
            /* [in] */ const VARIANT *params,
            /* [out] */ VARIANT *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ const VARIANT *item,
            /* [out] */ VARIANT *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Index( 
            /* [in] */ const VARIANT *index,
            /* [out] */ VARIANT *result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IObject * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *New )( 
            IObject * This,
            /* [out] */ VARIANT *result);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            IObject * This,
            /* [out] */ VARIANT *result);
        
        HRESULT ( STDMETHODCALLTYPE *Set )( 
            IObject * This,
            /* [in] */ const VARIANT *value);
        
        HRESULT ( STDMETHODCALLTYPE *Call )( 
            IObject * This,
            /* [in] */ const VARIANT *params,
            /* [out] */ VARIANT *result);
        
        HRESULT ( STDMETHODCALLTYPE *Item )( 
            IObject * This,
            /* [in] */ const VARIANT *item,
            /* [out] */ VARIANT *result);
        
        HRESULT ( STDMETHODCALLTYPE *Index )( 
            IObject * This,
            /* [in] */ const VARIANT *index,
            /* [out] */ VARIANT *result);
        
        END_INTERFACE
    } IObjectVtbl;

    interface IObject
    {
        CONST_VTBL struct IObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObject_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IObject_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IObject_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IObject_New(This,result)	\
    ( (This)->lpVtbl -> New(This,result) ) 

#define IObject_Get(This,result)	\
    ( (This)->lpVtbl -> Get(This,result) ) 

#define IObject_Set(This,value)	\
    ( (This)->lpVtbl -> Set(This,value) ) 

#define IObject_Call(This,params,result)	\
    ( (This)->lpVtbl -> Call(This,params,result) ) 

#define IObject_Item(This,item,result)	\
    ( (This)->lpVtbl -> Item(This,item,result) ) 

#define IObject_Index(This,index,result)	\
    ( (This)->lpVtbl -> Index(This,index,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IObject_INTERFACE_DEFINED__ */



#ifndef __NScript_LIBRARY_DEFINED__
#define __NScript_LIBRARY_DEFINED__

/* library NScript */
/* [uuid][version] */ 


EXTERN_C const IID LIBID_NScript;

EXTERN_C const CLSID CLSID_Parser;

#ifdef __cplusplus

class DECLSPEC_UUID("3fd55c79-34ce-4ba1-b86d-e39da245cbca")
Parser;
#endif
#endif /* __NScript_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


