// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

//#include "Aero.h"
#include "WtlAero.h"
#include "../NScriptHost/NScript/NScript.h"
#include "../NScriptHost/NScript/NHash.h"
#include "../NScriptHost/NScript/NDispatch.h"


class CMainDlg : 
	//public CAeroDialogImpl<CMainDlg>, 
	public aero::CDialogImpl<CMainDlg>, 
	public CWinDataExchange<CMainDlg>, 
	public CDialogResize<CMainDlg>,
	public CMessageFilter
{
/*
	CAeroStatic _resultCtrl;
	CAeroButton _okButton;
	CAeroButton _closeButton;
*/

	//CAeroEdit	_scriptCtrl;
	aero::CEdit		_scriptCtrl;

	CString		_script;
	CString		_result;
	CString		_result2;
	CString		_result3;

	CFont		_font;
	CFont		_fontResult;
	HACCEL		_hAccel;

public:
	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		/*if(_hAccel && TranslateAccelerator(m_hWnd, _hAccel, pMsg)) 
			return TRUE;*/
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_DLGRESIZE_MAP(CMainDlg)
		DLGRESIZE_CONTROL(IDC_SCRIPT, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_RESULT, DLSZ_SIZE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_RESULT2, DLSZ_SIZE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_RESULT3, DLSZ_SIZE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()

	BEGIN_DDX_MAP(CMainDlg)
		DDX_TEXT(IDC_SCRIPT, _script)
		DDX_TEXT(IDC_RESULT, _result)
		DDX_TEXT(IDC_RESULT2, _result2)
		DDX_TEXT(IDC_RESULT3, _result3)
		DDX_CONTROL(IDC_SCRIPT, _scriptCtrl)
	END_DDX_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		//CHAIN_MSG_MAP(CAeroDialogImpl<CMainDlg>)
		CHAIN_MSG_MAP(aero::CDialogImpl<CMainDlg>)
		CHAIN_MSG_MAP(CDialogResize<CMainDlg>)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, OnSelectAll)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		//__super::OnInitDialog(uMsg, wParam, lParam, bHandled);
		// center the dialog on the screen

		_hAccel=AtlLoadAccelerators(IDR_MAINFRAME);
		CenterWindow();

		// set icons
		HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
		SetIcon(hIcon, TRUE);
		HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		SetIcon(hIconSmall, FALSE);

		ExecuteDlgInit(IDD_MAINDLG);
		DlgResize_Init(false);

		MARGINS m = {-1};
		SetMargins(m);

		//_scriptCtrl.SubclassWindow(GetDlgItem(IDC_SCRIPT));
		//Subclass(_scriptCtrl, IDC_SCRIPT);

		//AERO_CONTROL(CEdit, _scriptCtrl ,IDC_SCRIPT);
		AERO_CONTROL(CStatic, resultCtrl ,IDC_RESULT);
		AERO_CONTROL(CStatic, result2Ctrl ,IDC_RESULT2);
		AERO_CONTROL(CStatic, result3Ctrl ,IDC_RESULT3);
		AERO_CONTROL(CButton, _okButton,IDOK);
		AERO_CONTROL(CButton, _closeButton,IDCANCEL);

		//_scriptCtrl.SubclassWindow(GetDlgItem(IDC_SCRIPT));
		/*_resultCtrl.SubclassWindow(GetDlgItem(IDC_RESULT));
		_okButton.SubclassWindow(GetDlgItem(IDOK));
		_closeButton.SubclassWindow(GetDlgItem(IDCANCEL));*/

		//SetOpaqueUnder(IDC_SCRIPT);

		DoDataExchange();
		//_scriptCtrl.SetMargins(2,2);
		_font.CreatePointFont(120, L"Consolas");
		_scriptCtrl.SetFont(_font);
		_fontResult.CreatePointFont(80, L"Consolas");
		result2Ctrl.SetFont(_fontResult);
		result3Ctrl.SetFont(_fontResult);

		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);

		return TRUE;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// unregister message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);

		return 0;
	}

	LRESULT OnSelectAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		_scriptCtrl.SetSelAll();
		return TRUE;
	}

	std::pair<DWORD, CString> GetErrorPos(CString msg, DWORD pos)
	{
		DWORD last_break = 0;
		for(unsigned i = 0; i < min(pos, (unsigned)msg.GetLength()); i++) {
			if(msg[i] == '\n' || msg[i] == '\r')	last_break = i + 1;
		}
		return { pos - last_break, msg.Mid(last_break) };
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(DoDataExchange(TRUE))	{
			nscript::NScript ns;
			variant_t res;
			ns.AddObject(TEXT("map"), (nscript::IHash*)new nscript::Hash());
			ns.AddObject(TEXT("createobject"), new nscript::CreateObject());
			HRESULT hr = ns.Exec(_script, res);
			_result2.Empty();
			_result3.Empty();
			if(FAILED(hr))	{
				IErrorInfoPtr pei;
				GetErrorInfo(0, &pei);
				_com_error e(hr, pei, true);
				if(e.HelpContext())		{
					_result.Format(TEXT("%s at pos %d:"), (LPCTSTR)e.Description(), e.HelpContext());
					auto err_pos = GetErrorPos((LPCTSTR)e.HelpFile(), e.HelpContext());
					_result2 = (LPCTSTR)err_pos.second;
					_result3 = CString(' ', err_pos.first-1) + '^';
				}	else	{
					_result = (LPCTSTR)e.Description();
				}
			}	else	{
				try			{
					if(V_VT(&res) & VT_ARRAY)	{
						nscript::SafeArray sa(res);
						_result = '[';
						for(int i=0; i<sa.Count(); i++)	{
							if(i)	_result += L" ; ";
							_result += bstr_t(sa[i]);
						}
						_result += ']';
					}	else	
						_result = (LPCTSTR)bstr_t(res);
				}
				catch(...)	{_result = L"Type mismatch.";	}
			}


			DoDataExchange();
			GetDlgItem(IDC_RESULT).Invalidate(FALSE);
			GetDlgItem(IDC_RESULT2).Invalidate(FALSE);
			GetDlgItem(IDC_RESULT3).Invalidate(FALSE);
			_scriptCtrl.SetSelAll();
			_scriptCtrl.SetFocus();
		}
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CloseDialog(wID);
		return 0;
	}

	void CloseDialog(int nVal)
	{
		DestroyWindow();
		::PostQuitMessage(nVal);
	}
	LRESULT OnBnClickedOk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
