#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "lhm.h"

#pragma comment(lib, "Comctl32.lib")

namespace
{
	constexpr wchar_t LHM_WINDOW_CLASS[] = L"HardwareMonitorWindow";
	constexpr wchar_t LHM_WINDOW_TITLE[] = L"LibreHardwareMonitor";
	constexpr UINT_PTR UPDATE_TIMER_ID = 1;
	constexpr UINT UPDATE_INTERVAL_MS = 1000;
	constexpr int COL_VAL_WIDTH = 60;
	constexpr int COL_MAX_WIDTH = 60;
	constexpr int COL_MIN_WIDTH = 60;
	constexpr int CELL_PADDING = 6;
	constexpr int LHM_WINDOW_WIDTH = 400;
	constexpr int LHM_WINDOW_HEIGHT = 600;
	constexpr UINT MENU_RUN_AS_ADMIN = 1000;
	constexpr UINT MENU_TOGGLE_BASE = 2000;

	HINSTANCE g_instance = nullptr;
	HWND g_main = nullptr;
	HWND g_header = nullptr;
	HWND g_tree = nullptr;
	HMENU g_menu_bar = nullptr;
	HMENU g_toggle_menu = nullptr;
	size_t g_sensor_count = 0;
	size_t g_toggle_count = 0;

	struct SensorNode
	{
		std::wstring id;
		std::wstring value_text;
		std::wstring max_text;
	};

	std::vector<std::unique_ptr<SensorNode>> g_nodes;
	std::unordered_map<std::wstring, SensorNode*> g_sensor_nodes;
	std::unordered_map<std::wstring, HTREEITEM> g_group_nodes;

	void UpdateHeaderColumns(int total_width);
	void UpdateHeaderColumnsForTree();
	bool IsAdmin();
	bool RelaunchElevated();

	std::wstring FormatReading(bool has_value, float value)
	{
		if (!has_value)
		{
			return L"n/a";
		}

		return std::format(L"{:.2f}", value);
	}

	void UpdateNodeFromSensor(SensorNode& node, const LhmSensorInfo& sensor)
	{
		node.value_text = FormatReading(sensor.has_value, sensor.value);
		node.max_text = FormatReading(sensor.has_max, sensor.max);
	}

	std::wstring MakeGroupKey(std::wstring_view hardware, std::wstring_view type)
	{
		std::wstring key;
		key.reserve(hardware.size() + type.size() + 1);
		key.append(hardware);
		key.push_back(L'|');
		key.append(type);
		return key;
	}

	void ClearTree()
	{
		TreeView_DeleteAllItems(g_tree);
		g_nodes.clear();
		g_sensor_nodes.clear();
		g_group_nodes.clear();
	}

	HTREEITEM InsertGroupItem(HTREEITEM parent, const std::wstring& key, const std::wstring& text)
	{
		auto it = g_group_nodes.find(key);
		if (it != g_group_nodes.end())
		{
			return it->second;
		}

		TVINSERTSTRUCTW insert{};
		insert.hParent = parent;
		insert.hInsertAfter = TVI_LAST;
		insert.item.mask = TVIF_TEXT;
		insert.item.pszText = const_cast<wchar_t*>(text.c_str());

		HTREEITEM item = TreeView_InsertItem(g_tree, &insert);
		g_group_nodes.emplace(key, item);
		return item;
	}

	void ExpandAllGroups()
	{
		for (const auto& entry : g_group_nodes)
		{
			TreeView_Expand(g_tree, entry.second, TVE_EXPAND);
		}
	}

	bool BuildTreeFromSensors()
	{
		const LhmSensorInfo* sensors = nullptr;
		size_t sensor_count = 0;
		if (!LhmEnumerateSensors(&sensors, &sensor_count))
		{
			MessageBoxW(g_main, LhmGetLastError(), L"Error", MB_ICONERROR);
			return false;
		}

		ClearTree();

		for (size_t i = 0; i < sensor_count; ++i)
		{
			const auto& sensor = sensors[i];
			std::wstring hardware = sensor.hardware;
			std::wstring type = sensor.type;

			HTREEITEM hardware_item = InsertGroupItem(TVI_ROOT, hardware, hardware);
			std::wstring type_key = MakeGroupKey(hardware, type);
			HTREEITEM type_item = InsertGroupItem(hardware_item, type_key, type);

			auto node = std::make_unique<SensorNode>();
			node->id = sensor.id;
			UpdateNodeFromSensor(*node, sensor);

			SensorNode* node_ptr = node.get();
			g_nodes.push_back(std::move(node));
			g_sensor_nodes.emplace(node_ptr->id, node_ptr);

			TVINSERTSTRUCTW insert{};
			insert.hParent = type_item;
			insert.hInsertAfter = TVI_LAST;
			insert.item.mask = TVIF_TEXT | TVIF_PARAM;
			insert.item.pszText = const_cast<wchar_t*>(sensor.name);
			insert.item.lParam = reinterpret_cast<LPARAM>(node_ptr);
			TreeView_InsertItem(g_tree, &insert);
		}

		ExpandAllGroups();
		HTREEITEM root = TreeView_GetRoot(g_tree);
		if (root)
		{
			TreeView_EnsureVisible(g_tree, root);
		}
		UpdateHeaderColumnsForTree();
		g_sensor_count = sensor_count;
		return true;
	}

	bool UpdateValues()
	{
		const LhmSensorInfo* sensors = nullptr;
		size_t sensor_count = 0;
		if (!LhmEnumerateSensors(&sensors, &sensor_count))
		{
			return false;
		}

		bool rebuild = sensor_count != g_sensor_count;
		if (!rebuild)
		{
			for (size_t i = 0; i < sensor_count; ++i)
			{
				if (g_sensor_nodes.find(sensors[i].id) == g_sensor_nodes.end())
				{
					rebuild = true;
					break;
				}
			}
		}

		if (rebuild)
		{
			return BuildTreeFromSensors();
		}

		for (size_t i = 0; i < sensor_count; ++i)
		{
			const auto& sensor = sensors[i];
			auto it = g_sensor_nodes.find(sensor.id);
			if (it == g_sensor_nodes.end())
			{
				continue;
			}

			UpdateNodeFromSensor(*it->second, sensor);
		}

		InvalidateRect(g_tree, nullptr, FALSE);
		return true;
	}

	void InitHeaderColumns()
	{
		HDITEMW item{};
		item.mask = HDI_TEXT | HDI_WIDTH;

		item.cxy = COL_MIN_WIDTH;
		item.pszText = const_cast<wchar_t*>(L"Sensor");
		Header_InsertItem(g_header, 0, &item);

		item.cxy = COL_VAL_WIDTH;
		item.pszText = const_cast<wchar_t*>(L"Value");
		Header_InsertItem(g_header, 1, &item);

		item.cxy = COL_MAX_WIDTH;
		item.pszText = const_cast<wchar_t*>(L"Max");
		Header_InsertItem(g_header, 2, &item);
	}

	void UpdateHeaderColumns(int total_width)
	{
		const int reserved = COL_VAL_WIDTH + COL_MAX_WIDTH;
		int sensor_width = total_width - reserved;
		if (sensor_width < COL_MIN_WIDTH)
		{
			sensor_width = COL_MIN_WIDTH;
		}

		HDITEMW item{};
		item.mask = HDI_WIDTH;

		item.cxy = sensor_width;
		Header_SetItem(g_header, 0, &item);

		item.cxy = COL_VAL_WIDTH;
		Header_SetItem(g_header, 1, &item);

		item.cxy = COL_MAX_WIDTH;
		Header_SetItem(g_header, 2, &item);
	}

	void UpdateHeaderColumnsForTree()
	{
		if (!g_tree)
		{
			return;
		}

		RECT tree_rect{};
		GetClientRect(g_tree, &tree_rect);
		const int width = tree_rect.right - tree_rect.left;
		if (width <= 0)
		{
			return;
		}

		UpdateHeaderColumns(width);
	}

	bool IsAdmin()
	{
		BOOL is_admin = FALSE;
		SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
		PSID admin_group = nullptr;
		if (AllocateAndInitializeSid(&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
				0, 0, 0, 0, 0, 0, &admin_group))
		{
			if (!CheckTokenMembership(nullptr, admin_group, &is_admin))
			{
				is_admin = FALSE;
			}
			FreeSid(admin_group);
		}
		return is_admin != FALSE;
	}

	std::wstring BuildRelaunchParameters()
	{
		int argc = 0;
		LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		if (!argv || argc <= 1)
		{
			if (argv)
			{
				LocalFree(argv);
			}
			return L"";
		}

		std::wstring params;
		for (int i = 1; i < argc; ++i)
		{
			if (i > 1)
			{
				params.push_back(L' ');
			}
			params.push_back(L'"');
			params.append(argv[i]);
			params.push_back(L'"');
		}
		LocalFree(argv);
		return params;
	}

	bool RelaunchElevated()
	{
		WCHAR prog[MAX_PATH]{};
		if (!GetModuleFileNameW(nullptr, prog, MAX_PATH))
		{
			return false;
		}

		const std::wstring params = BuildRelaunchParameters();

		SHELLEXECUTEINFOW sei{};
		sei.cbSize = sizeof(sei);
		sei.lpVerb = L"runas";
		sei.lpFile = prog;
		sei.lpParameters = params.empty() ? nullptr : params.c_str();
		sei.nShow = SW_SHOWDEFAULT;

		return ShellExecuteExW(&sei) != FALSE;
	}


	void LayoutControls(HWND hwnd)
	{
		RECT client{};
		GetClientRect(hwnd, &client);

		RECT header_rect = client;
		HDLAYOUT layout{};
		layout.prc = &header_rect;

		WINDOWPOS header_pos{};
		layout.pwpos = &header_pos;

		Header_Layout(g_header, &layout);
		SetWindowPos(g_header, header_pos.hwndInsertAfter, header_pos.x, header_pos.y, header_pos.cx, header_pos.cy, header_pos.flags);

		const int width = client.right - client.left;
		const int height = client.bottom - client.top - header_pos.cy;
		SetWindowPos(g_tree, nullptr, 0, header_pos.cy, width, height, SWP_NOZORDER);

		UpdateHeaderColumnsForTree();
	}

	LRESULT HandleTreeCustomDraw(const NMTVCUSTOMDRAW* draw)
	{
		switch (draw->nmcd.dwDrawStage)
		{
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;
		case CDDS_ITEMPREPAINT:
			return CDRF_NOTIFYPOSTPAINT;
		case CDDS_ITEMPOSTPAINT:
			break;
		default:
			return CDRF_DODEFAULT;
		}

		HTREEITEM item = reinterpret_cast<HTREEITEM>(draw->nmcd.dwItemSpec);
		RECT text_rect{};
		if (!TreeView_GetItemRect(g_tree, item, &text_rect, TRUE))
		{
			return CDRF_DODEFAULT;
		}

		RECT value_col{};
		RECT max_col{};
		if (!Header_GetItemRect(g_header, 1, &value_col) || !Header_GetItemRect(g_header, 2, &max_col))
		{
			return CDRF_DODEFAULT;
		}

		const bool selected = (draw->nmcd.uItemState & CDIS_SELECTED) != 0;
		const COLORREF text_color = GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
		HBRUSH brush = GetSysColorBrush(selected ? COLOR_HIGHLIGHT : COLOR_WINDOW);

		SensorNode* node = reinterpret_cast<SensorNode*>(draw->nmcd.lItemlParam);
		std::wstring_view value_text;
		std::wstring_view max_text;
		if (node != nullptr)
		{
			value_text = node->value_text;
			max_text = node->max_text;
		}

		HDC hdc = draw->nmcd.hdc;
		const int old_bk_mode = SetBkMode(hdc, TRANSPARENT);
		const COLORREF old_text_color = SetTextColor(hdc, text_color);

		RECT value_rect{ value_col.left, text_rect.top, value_col.right, text_rect.bottom };
		RECT max_rect{ max_col.left, text_rect.top, max_col.right, text_rect.bottom };

		FillRect(hdc, &value_rect, brush);
		FillRect(hdc, &max_rect, brush);

		value_rect.left += CELL_PADDING;
		value_rect.right -= CELL_PADDING;
		max_rect.left += CELL_PADDING;
		max_rect.right -= CELL_PADDING;

		const wchar_t* value_ptr = value_text.empty() ? L"" : value_text.data();
		const wchar_t* max_ptr = max_text.empty() ? L"" : max_text.data();

		DrawTextW(hdc, value_ptr, static_cast<int>(value_text.size()), &value_rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		DrawTextW(hdc, max_ptr, static_cast<int>(max_text.size()), &max_rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

		SetTextColor(hdc, old_text_color);
		SetBkMode(hdc, old_bk_mode);

		return CDRF_DODEFAULT;
	}

	bool CreateControls(HWND hwnd)
	{
		g_header = CreateWindowExW(0, WC_HEADERW, nullptr, WS_CHILD | WS_VISIBLE | HDS_HORZ, 0, 0, 0, 0, hwnd, nullptr, g_instance, nullptr);
		if (!g_header)
		{
			return false;
		}

		g_tree = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, nullptr, WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_FULLROWSELECT | TVS_SHOWSELALWAYS | TVS_NOHSCROLL | TVS_INFOTIP, 0, 0, 0, 0, hwnd, nullptr, g_instance, nullptr);
		if (!g_tree)
		{
			return false;
		}

		HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
		SendMessageW(g_header, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
		SendMessageW(g_tree, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

		InitHeaderColumns();
		return true;
	}

	bool CreateMenuBar(HWND hwnd)
	{
		HMENU menu_bar = CreateMenu();
		if (!menu_bar)
		{
			return false;
		}

		HMENU toggle_menu = CreatePopupMenu();
		if (!toggle_menu)
		{
			DestroyMenu(menu_bar);
			return false;
		}

		HMENU file_menu = CreatePopupMenu();
		if (!file_menu)
		{
			DestroyMenu(toggle_menu);
			DestroyMenu(menu_bar);
			return false;
		}

		const UINT admin_flags = MF_STRING | (IsAdmin() ? MF_GRAYED : MF_ENABLED);
		AppendMenuW(file_menu, admin_flags, MENU_RUN_AS_ADMIN, L"Run as &Administrator");
		AppendMenuW(menu_bar, MF_POPUP, reinterpret_cast<UINT_PTR>(file_menu), L"&File");

		g_toggle_count = LhmGetToggleCount();
		for (size_t i = 0; i < g_toggle_count; ++i)
		{
			const wchar_t* name = nullptr;
			bool enabled = false;
			if (!LhmGetToggleInfo(i, &name, &enabled))
			{
				DestroyMenu(toggle_menu);
				DestroyMenu(menu_bar);
				return false;
			}

			const UINT flags = MF_STRING | (enabled ? MF_CHECKED : MF_UNCHECKED);
			AppendMenuW(toggle_menu, flags, MENU_TOGGLE_BASE + static_cast<UINT>(i), name);
		}

		AppendMenuW(menu_bar, MF_POPUP, reinterpret_cast<UINT_PTR>(toggle_menu), L"&Hardware");

		if (!SetMenu(hwnd, menu_bar))
		{
			DestroyMenu(toggle_menu);
			DestroyMenu(menu_bar);
			return false;
		}

		DrawMenuBar(hwnd);

		g_menu_bar = menu_bar;
		g_toggle_menu = toggle_menu;
		return true;
	}

	LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param)
	{
		switch (message)
		{
		case WM_CREATE:
			g_main = hwnd;
			if (!CreateControls(hwnd))
			{
				return -1;
			}
			if (!CreateMenuBar(hwnd))
			{
				MessageBoxW(hwnd, L"Failed to create menu.", L"Error", MB_ICONERROR);
				return -1;
			}
			if (!BuildTreeFromSensors())
			{
				return -1;
			}
			SetTimer(hwnd, UPDATE_TIMER_ID, UPDATE_INTERVAL_MS, nullptr);
			return 0;
		case WM_SIZE:
			LayoutControls(hwnd);
			return 0;
		case WM_TIMER:
			if (w_param == UPDATE_TIMER_ID)
			{
				UpdateValues();
			}
			return 0;
		case WM_COMMAND:
		{
			const UINT command_id = LOWORD(w_param);
			if (command_id == MENU_RUN_AS_ADMIN)
			{
				if (!IsAdmin() && RelaunchElevated())
				{
					PostQuitMessage(0);
				}
				return 0;
			}
			if (command_id >= MENU_TOGGLE_BASE && command_id < MENU_TOGGLE_BASE + g_toggle_count)
			{
				const size_t toggle_index = command_id - MENU_TOGGLE_BASE;
				const wchar_t* name = nullptr;
				bool enabled = false;
				if (!LhmGetToggleInfo(toggle_index, &name, &enabled))
				{
					MessageBoxW(hwnd, LhmGetLastError(), L"Error", MB_ICONERROR);
					return 0;
				}

				const bool new_value = !enabled;
				if (!LhmSetToggleEnabled(toggle_index, new_value))
				{
					MessageBoxW(hwnd, LhmGetLastError(), L"Error", MB_ICONERROR);
					return 0;
				}

				CheckMenuItem(g_toggle_menu, command_id, MF_BYCOMMAND | (new_value ? MF_CHECKED : MF_UNCHECKED));
				BuildTreeFromSensors();
				return 0;
			}
			break;
		}
		case WM_NOTIFY:
		{
			const NMHDR* header = reinterpret_cast<const NMHDR*>(l_param);
			if (header->hwndFrom == g_tree)
			{
				if (header->code == NM_CUSTOMDRAW)
				{
					return HandleTreeCustomDraw(reinterpret_cast<const NMTVCUSTOMDRAW*>(l_param));
				}
				if (header->code == TVN_GETINFOTIPW)
				{
					auto* info = reinterpret_cast<NMTVGETINFOTIPW*>(l_param);
					auto* node = reinterpret_cast<SensorNode*>(info->lParam);
					if (info->pszText && info->cchTextMax > 0)
					{
						if (node)
						{
							wcsncpy_s(info->pszText, info->cchTextMax, node->id.c_str(), _TRUNCATE);
						}
						else
						{
							info->pszText[0] = L'\0';
						}
					}
					return 0;
				}
			}
			if (header->hwndFrom == g_header)
			{
				switch (header->code)
				{
				case HDN_BEGINTRACK:
				case HDN_TRACK:
				case HDN_ENDTRACK:
				case HDN_ITEMCHANGING:
				case HDN_ITEMCHANGED:
					RedrawWindow(g_tree, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
					return 0;
				}
			}
			break;
		}
		case WM_DESTROY:
			KillTimer(hwnd, UPDATE_TIMER_ID);
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProcW(hwnd, message, w_param, l_param);
	}
}

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd
)
{
	g_instance = hInstance;

	INITCOMMONCONTROLSEX controls{};
	controls.dwSize = sizeof(controls);
	controls.dwICC = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&controls);

	if (!LhmInitialize())
	{
		MessageBoxW(nullptr, LhmGetLastError(), L"Error", MB_ICONERROR);
		return 1;
	}

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWindowProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszClassName = LHM_WINDOW_CLASS;
	wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

	if (!RegisterClassExW(&wc))
	{
		LhmShutdown();
		return 1;
	}

	HWND hwnd = CreateWindowExW(0, LHM_WINDOW_CLASS, LHM_WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, LHM_WINDOW_WIDTH, LHM_WINDOW_HEIGHT, nullptr, nullptr, hInstance, nullptr);
	if (!hwnd)
	{
		LhmShutdown();
		return 1;
	}

	ShowWindow(hwnd, nShowCmd);
	UpdateWindow(hwnd);

	MSG msg{};
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	LhmShutdown();
	return static_cast<int>(msg.wParam);
}
