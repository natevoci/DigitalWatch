/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Sep 23 19:36:51 2004
 */
/* Compiler settings for .\dsnetifc.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
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

#ifndef __dsnetifc_h__
#define __dsnetifc_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IMulticastConfig_FWD_DEFINED__
#define __IMulticastConfig_FWD_DEFINED__
typedef interface IMulticastConfig IMulticastConfig;
#endif 	/* __IMulticastConfig_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_dsnetifc_0000 */
/* [local] */ 

// {a07e6137-6c07-45d9-a00c-7de7a7e6319b}
DEFINE_GUID(CLSID_DSNetSend,
0xa07e6137, 0x6c07, 0x45d9, 0xa0, 0x0c, 0x7d, 0xe7, 0xa7, 0xe6, 0x31, 0x9b);
// {4ce1b653-cb0d-4eba-a723-01026bb5ff13}
DEFINE_GUID(CLSID_DSNetReceive,
0x4ce1b653, 0xcb0d, 0x4eba, 0xa7, 0x23, 0x01, 0x02, 0x6b, 0xb5, 0xff, 0x13);
// {fa966959-7498-4f16-8585-cc1b688501cd}
DEFINE_GUID(CLSID_IPMulticastSendProppage,
0xfa966959, 0x7498, 0x4f16, 0x85, 0x85, 0xcc, 0x1b, 0x68, 0x85, 0x01, 0xcd);
// {b8753014-fd31-4a3e-929f-982dc29d6f4a}
DEFINE_GUID(CLSID_IPMulticastRecvProppage,
0xb8753014, 0xfd31, 0x4a3e, 0x92, 0x9f, 0x98, 0x2d, 0xc2, 0x9d, 0x6f, 0x4a);



extern RPC_IF_HANDLE __MIDL_itf_dsnetifc_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dsnetifc_0000_v0_0_s_ifspec;

#ifndef __IMulticastConfig_INTERFACE_DEFINED__
#define __IMulticastConfig_INTERFACE_DEFINED__

/* interface IMulticastConfig */
/* [uuid][object] */ 


EXTERN_C const IID IID_IMulticastConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1CB42CC8-D32C-4f73-9267-C114DA470378")
    IMulticastConfig : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetNetworkInterface( 
            /* [in] */ ULONG ulNIC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNetworkInterface( 
            /* [out] */ ULONG __RPC_FAR *pNIC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMulticastGroup( 
            /* [in] */ ULONG ulIP,
            /* [in] */ USHORT usPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMulticastGroup( 
            /* [out] */ ULONG __RPC_FAR *pIP,
            /* [out] */ USHORT __RPC_FAR *pPort) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMulticastConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMulticastConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMulticastConfig __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMulticastConfig __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNetworkInterface )( 
            IMulticastConfig __RPC_FAR * This,
            /* [in] */ ULONG ulNIC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNetworkInterface )( 
            IMulticastConfig __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pNIC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMulticastGroup )( 
            IMulticastConfig __RPC_FAR * This,
            /* [in] */ ULONG ulIP,
            /* [in] */ USHORT usPort);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMulticastGroup )( 
            IMulticastConfig __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pIP,
            /* [out] */ USHORT __RPC_FAR *pPort);
        
        END_INTERFACE
    } IMulticastConfigVtbl;

    interface IMulticastConfig
    {
        CONST_VTBL struct IMulticastConfigVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMulticastConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMulticastConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMulticastConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMulticastConfig_SetNetworkInterface(This,ulNIC)	\
    (This)->lpVtbl -> SetNetworkInterface(This,ulNIC)

#define IMulticastConfig_GetNetworkInterface(This,pNIC)	\
    (This)->lpVtbl -> GetNetworkInterface(This,pNIC)

#define IMulticastConfig_SetMulticastGroup(This,ulIP,usPort)	\
    (This)->lpVtbl -> SetMulticastGroup(This,ulIP,usPort)

#define IMulticastConfig_GetMulticastGroup(This,pIP,pPort)	\
    (This)->lpVtbl -> GetMulticastGroup(This,pIP,pPort)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMulticastConfig_SetNetworkInterface_Proxy( 
    IMulticastConfig __RPC_FAR * This,
    /* [in] */ ULONG ulNIC);


void __RPC_STUB IMulticastConfig_SetNetworkInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMulticastConfig_GetNetworkInterface_Proxy( 
    IMulticastConfig __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pNIC);


void __RPC_STUB IMulticastConfig_GetNetworkInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMulticastConfig_SetMulticastGroup_Proxy( 
    IMulticastConfig __RPC_FAR * This,
    /* [in] */ ULONG ulIP,
    /* [in] */ USHORT usPort);


void __RPC_STUB IMulticastConfig_SetMulticastGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMulticastConfig_GetMulticastGroup_Proxy( 
    IMulticastConfig __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pIP,
    /* [out] */ USHORT __RPC_FAR *pPort);


void __RPC_STUB IMulticastConfig_GetMulticastGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMulticastConfig_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dsnetifc_0007 */
/* [local] */ 

#define DECLARE_IMULTICASTCONFIG();\
virtual STDMETHODIMP SetNetworkInterface (ULONG) ;\
virtual STDMETHODIMP GetNetworkInterface (ULONG *) ;\
virtual STDMETHODIMP SetMulticastGroup (ULONG, USHORT) ;\
virtual STDMETHODIMP GetMulticastGroup (ULONG *, USHORT*) ;


extern RPC_IF_HANDLE __MIDL_itf_dsnetifc_0007_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dsnetifc_0007_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
