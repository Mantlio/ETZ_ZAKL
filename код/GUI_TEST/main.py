import dearpygui.dearpygui as dpg
import math
import re

dpg.create_context()

FIELD_W, FIELD_H = 26.6, 17.0
RATIO = FIELD_W / FIELD_H
MATH_CTX = {"sin": math.sin, "cos": math.cos, "tan": math.tan, "sqrt": math.sqrt, "abs": abs, "pow": pow, "PI": math.pi, "pi": math.pi}

status, fl, flr, sx, sy, c_x, c_y, arc_step = 0, 0, 0, 0, 0, 0, 0, 0
current_path_pts = [] 
undo_stack, redo_stack = [""], []

with dpg.font_registry():
    try:
        with dpg.font(r"C:\Windows\Fonts\arial.ttf", 20, default_font=True, tag="Default_font"):
            dpg.add_font_range_hint(dpg.mvFontRangeHint_Cyrillic)
        dpg.bind_font("Default_font")
    except: pass

def safe_eval(expr):
    try:
        clean_expr = re.sub(r'([0-9.]+)f', r'\1', str(expr)).strip()
        return float(eval(clean_expr, {"__builtins__": None}, MATH_CTX)) if clean_expr else 0.0
    except: return 0.0

def save_history():
    current_code = dpg.get_value("side_code_output")
    if not undo_stack or undo_stack[-1] != current_code:
        undo_stack.append(current_code); redo_stack.clear()
        if len(undo_stack) > 100: undo_stack.pop(0)

def undo():
    if len(undo_stack) > 1:
        redo_stack.append(undo_stack.pop()); dpg.set_value("side_code_output", undo_stack[-1]); sync_code_to_graph()

def redo():
    if redo_stack:
        state = redo_stack.pop(); undo_stack.append(state); dpg.set_value("side_code_output", state); sync_code_to_graph()

def get_stepped_speed():
    return round(dpg.get_value("speed_input") / 500) * 500

def get_snapped_pos():
    pos = dpg.get_plot_mouse_pos()
    snap = 1.0 if dpg.get_value("grid_snap_cb") else 0.1
    return round(pos[0] / snap) * snap, round(pos[1] / snap) * snap

def add_to_code(new_line):
    prefix = "// " if dpg.get_value("aux_mode_cb") else ""
    cur = dpg.get_value("side_code_output").strip()
    processed = "\n".join([(prefix + ln) for ln in new_line.split("\n")]) if "\n" in new_line else prefix + new_line
    dpg.set_value("side_code_output", (cur + "\n" + processed).strip() if cur else processed)
    save_history(); sync_code_to_graph()

def sync_code_to_graph():
    code = dpg.get_value("side_code_output"); lines = code.split('\n')
    p_x, p_y, l_x, l_y, path_x, path_y = [], [], [], [], [], []
    aux_p_x, aux_p_y, aux_l_x, aux_l_y = [], [], [], []
    arrays = {}
    for line in lines:
        m_arr = re.search(r"Point\s+(\w+)\[\]\s*=\s*\{(.*?)\};", line)
        if m_arr:
            name = m_arr.group(1)
            pts_content = re.findall(r"\{\s*([^,]+)\s*,\s*([^,]+)(?:\s*,\s*[^}]+)?\s*\}", m_arr.group(2))
            arrays[name] = [(safe_eval(x), -safe_eval(y)) for x, y in pts_content]
    for line in lines:
        is_aux = line.strip().startswith("//"); cl = line.replace("//", "").strip()
        if m := re.search(r"draw_point\(\s*([^,]+)\s*,\s*([^)]+)\s*\)", cl):
            tx, ty = safe_eval(m.group(1)), -safe_eval(m.group(2))
            if is_aux: aux_p_x.append(tx); aux_p_y.append(ty)
            else: p_x.append(tx); p_y.append(ty)
        elif m := re.search(r"draw_line\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^)]+)\s*\)", cl):
            pts, ypts = [safe_eval(m.group(1)), safe_eval(m.group(3)), float('nan')], [-safe_eval(m.group(2)), -safe_eval(m.group(4)), float('nan')]
            if is_aux: aux_l_x.extend(pts); aux_l_y.extend(ypts)
            else: l_x.extend(pts); l_y.extend(ypts)
        elif m := re.search(r"draw_rect\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^)]+)\s*\)", cl):
            x1, y1, x2, y2 = safe_eval(m.group(1)), -safe_eval(m.group(2)), safe_eval(m.group(3)), -safe_eval(m.group(4))
            rx, ry = [x1, x2, x2, x1, x1, float('nan')], [y1, y1, y2, y2, y1, float('nan')]
            if is_aux: aux_l_x.extend(rx); aux_l_y.extend(ry)
            else: l_x.extend(rx); l_y.extend(ry)
        elif m := re.search(r"draw_circle\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^)]+)\s*\)", cl):
            cx, cy, r = safe_eval(m.group(1)), -safe_eval(m.group(2)), safe_eval(m.group(3))
            pts_c_x = [cx + r * math.cos(2*math.pi*i/60) for i in range(61)]; pts_c_y = [cy + r * math.sin(2*math.pi*i/60) for i in range(61)]
            if is_aux: aux_l_x.extend(pts_c_x + [float('nan')]); aux_l_y.extend(pts_c_y + [float('nan')])
            else: l_x.extend(pts_c_x + [float('nan')]); l_y.extend(pts_c_y + [float('nan')])
        elif (m := re.search(r"draw_path\(\s*(\w+)\s*,\s*[^)]+\s*\)", cl)) and m.group(1) in arrays:
            px = [p[0] for p in arrays[m.group(1)]]; py = [p[1] for p in arrays[m.group(1)]]
            if is_aux: aux_l_x.extend(px + [float('nan')]); aux_l_y.extend(py + [float('nan')])
            else: path_x.extend(px + [float('nan')]); path_y.extend(py + [float('nan')])
    dpg.set_value("scatter_series", [p_x, p_y]); dpg.set_value("line_series", [l_x, l_y])
    dpg.set_value("lom_series", [path_x, path_y]); dpg.set_value("aux_scatter", [aux_p_x, aux_p_y]); dpg.set_value("aux_line", [aux_l_x, aux_l_y])

def plot_callback():
    global status, fl, sx, sy, flr, c_x, c_y, current_path_pts, arc_step
    if not dpg.is_item_hovered("plot_widget"): return
    gx, gy = get_snapped_pos()
    if dpg.is_mouse_button_released(dpg.mvMouseButton_Right) and status == 3:
        if len(current_path_pts) > 1:
            pts_s = ", ".join([f"{{ {p[0]:.3f}, {abs(p[1]):.3f}, {p[2]} }}" for p in current_path_pts])
            idx = dpg.get_value("side_code_output").count("draw_path") + 1
            add_to_code(f"Point lom{idx}[] = {{ {pts_s} }};\ndraw_path(lom{idx}, 0);")
        current_path_pts.clear(); dpg.set_value("temp_lom_series", [[], []])
    if dpg.is_mouse_button_released(dpg.mvMouseButton_Left):
        if status == 1: add_to_code(f"draw_point({gx:.3f}, {abs(gy):.3f});")
        elif status == 2:
            if fl == 0: sx, sy, fl = gx, gy, 1
            else: add_to_code(f"draw_line({sx:.3f}, {abs(sy):.3f}, {gx:.3f}, {abs(gy):.3f});"); fl = 0
        elif status == 3: current_path_pts.append((gx, gy, get_stepped_speed()))
        elif status == 4:
            if flr == 0: c_x, c_y, flr = gx, gy, 1
            else:
                r = math.sqrt((gx - c_x)**2 + (gy - c_y)**2); add_to_code(f"draw_circle({c_x:.3f}, {abs(c_y):.3f}, {r:.3f});"); flr = 0
        elif status == 5:
            if fl == 0: sx, sy, fl = gx, gy, 1
            else: add_to_code(f"draw_rect({sx:.3f}, {abs(sy):.3f}, {gx:.3f}, {abs(gy):.3f});"); fl = 0
        elif status == 6:
            if flr == 0: c_x, c_y, flr = gx, gy, 1
            else:
                r, sides = math.sqrt((gx - c_x)**2 + (gy - c_y)**2), dpg.get_value("poly_sides")
                angle = math.atan2(gy - c_y, gx - c_x); pts = []
                for i in range(sides + 1):
                    a = angle + 2 * math.pi * i / sides
                    pts.append(f"{{ {c_x + r * math.cos(a):.3f}, {abs(c_y + r * math.sin(a)):.3f}, 0 }}")
                idx = dpg.get_value("side_code_output").count("draw_path") + 1
                add_to_code(f"Point poly{idx}[] = {{ {', '.join(pts)} }};\ndraw_path(poly{idx}, 0);"); flr = 0
        elif status == 7:
            if arc_step == 0: c_x, c_y, arc_step = gx, gy, 1
            elif arc_step == 1: sx, sy, arc_step = gx, gy, 2
            else:
                r = math.sqrt((sx - c_x)**2 + (sy - c_y)**2)
                a1, a2 = math.atan2(sy - c_y, sx - c_x), math.atan2(gy - c_y, gx - c_x)
                if a2 < a1: a2 += 2 * math.pi
                diff = a2 - a1; segments = max(10, int(r * diff * 15))
                pts = []
                for i in range(segments + 1):
                    a = a1 + diff * i / segments
                    pts.append(f"{{ {c_x + r * math.cos(a):.3f}, {abs(c_y + r * math.sin(a)):.3f}, 0 }}")
                idx = dpg.get_value("side_code_output").count("draw_path") + 1
                add_to_code(f"Point arc{idx}[] = {{ {', '.join(pts)} }};\ndraw_path(arc{idx}, 0);"); arc_step = 0
        dpg.set_value("temp_lom_series", [[], []])

def mouse_move_callback():
    global status, fl, sx, sy, flr, c_x, c_y, arc_step
    if dpg.is_item_hovered("plot_widget"):
        gx, gy = get_snapped_pos()
        dpg.configure_item("mouse_coord_tag", label=f"{gx:.1f}, {abs(gy):.1f}", default_value=[gx, gy], show=True)
        if status == 2 and fl == 1: dpg.set_value("temp_lom_series", [[sx, gx], [sy, gy]])
        elif status == 5 and fl == 1: dpg.set_value("temp_lom_series", [[sx, gx, gx, sx, sx], [sy, sy, gy, gy, sy]])
        elif status == 4 and flr == 1:
            r = math.sqrt((gx-c_x)**2 + (gy-c_y)**2); tx = [c_x + r * math.cos(2*math.pi*i/60) for i in range(61)]; ty = [c_y + r * math.sin(2*math.pi*i/60) for i in range(61)]
            dpg.set_value("temp_lom_series", [tx, ty])
        elif status == 6 and flr == 1:
            r, sides = math.sqrt((gx - c_x)**2 + (gy - c_y)**2), dpg.get_value("poly_sides")
            angle = math.atan2(gy - c_y, gx - c_x); tx = [c_x + r * math.cos(angle + 2*math.pi*i/sides) for i in range(sides + 1)]; ty = [c_y + r * math.sin(angle + 2*math.pi*i/sides) for i in range(sides + 1)]
            dpg.set_value("temp_lom_series", [tx, ty])
        elif status == 3 and len(current_path_pts) > 0:
            tx = [p[0] for p in current_path_pts] + [gx]; ty = [p[1] for p in current_path_pts] + [gy]
            dpg.set_value("temp_lom_series", [tx, ty])
        elif status == 7:
            if arc_step == 1:
                r = math.sqrt((gx - c_x)**2 + (gy - c_y)**2)
                tx = [c_x + r * math.cos(2*math.pi*i/60) for i in range(61)]; ty = [c_y + r * math.sin(2*math.pi*i/60) for i in range(61)]
                dpg.set_value("temp_lom_series", [tx, ty])
            elif arc_step == 2:
                r = math.sqrt((sx - c_x)**2 + (sy - c_y)**2); a1, a2 = math.atan2(sy - c_y, sx - c_x), math.atan2(gy - c_y, gx - c_x)
                if a2 < a1: a2 += 2 * math.pi
                diff = a2 - a1; segments = max(10, int(r * diff * 15))
                tx = [c_x + r * math.cos(a1 + diff * i / segments) for i in range(segments + 1)]
                ty = [c_y + r * math.sin(a1 + diff * i / segments) for i in range(segments + 1)]
                dpg.set_value("temp_lom_series", [tx, ty])
    else: dpg.configure_item("mouse_coord_tag", show=False)

def abort_drawing():
    global fl, flr, current_path_pts, arc_step
    fl, flr, arc_step = 0, 0, 0
    current_path_pts.clear(); dpg.set_value("temp_lom_series", [[], []])

def resize_handler():
    h = dpg.get_viewport_height() - 60; dpg.configure_item("graph_container", width=int(h * RATIO))

def open_poly_settings():
    dpg.configure_item("poly_settings_win", show=True)

with dpg.theme() as main_theme:
    with dpg.theme_component(dpg.mvLineSeries): dpg.add_theme_color(dpg.mvPlotCol_Line, (0, 200, 255, 255), category=dpg.mvThemeCat_Plots)
with dpg.theme() as aux_theme:
    with dpg.theme_component(dpg.mvLineSeries): dpg.add_theme_color(dpg.mvPlotCol_Line, (180, 100, 255, 255), category=dpg.mvThemeCat_Plots)
with dpg.theme() as temp_theme:
    with dpg.theme_component(dpg.mvLineSeries): dpg.add_theme_color(dpg.mvPlotCol_Line, (255, 255, 255, 150), category=dpg.mvThemeCat_Plots)

with dpg.window(label="Настройка многоугольника", modal=True, show=False, tag="poly_settings_win", pos=[500, 300]):
    dpg.add_text("Выберите количество углов:")
    dpg.add_slider_int(tag="poly_sides", default_value=6, min_value=3, max_value=20, width=250)
    with dpg.group(horizontal=True):
        dpg.add_button(label="ОК", width=120, callback=lambda: (globals().update(status=6, flr=0), dpg.configure_item("poly_settings_win", show=False)))
        dpg.add_button(label="Отмена", width=120, callback=lambda: dpg.configure_item("poly_settings_win", show=False))

with dpg.window(tag="PrimaryWindow"):
    with dpg.menu_bar():
        with dpg.menu(label="Файл"):
            dpg.add_menu_item(label="Новый", callback=lambda: (dpg.set_value("side_code_output", ""), save_history(), sync_code_to_graph()))
            dpg.add_menu_item(label="Полный экран (F11)", callback=lambda: dpg.toggle_viewport_fullscreen()); dpg.add_menu_item(label="Закрыть", callback=lambda: dpg.stop_dearpygui())
        with dpg.menu(label="Геометрия"):
            dpg.add_menu_item(label="Точка", callback=lambda: globals().update(status=1))
            dpg.add_menu_item(label="Линия", callback=lambda: globals().update(status=2, fl=0))
            dpg.add_menu_item(label="Ломанная", callback=lambda: globals().update(status=3, current_path_pts=[]))
            dpg.add_menu_item(label="Круг", callback=lambda: globals().update(status=4, flr=0))
            dpg.add_menu_item(label="Прямоугольник", callback=lambda: globals().update(status=5, fl=0))
            dpg.add_menu_item(label="Многоугольник", callback=open_poly_settings)
            dpg.add_menu_item(label="Дуга", callback=lambda: globals().update(status=7, arc_step=0))
        with dpg.menu(label="Команды"):
            dpg.add_menu_item(label="Доехать до x", callback=lambda: add_to_code(f"go_line_x({get_stepped_speed()});"))
            dpg.add_menu_item(label="Доехать до y", callback=lambda: add_to_code(f"go_line_y({get_stepped_speed()});"))
            dpg.add_menu_item(label="Перемещение (x, y)", callback=lambda: add_to_code(f"move({get_snapped_pos()[0]:.1f}, {abs(get_snapped_pos()[1]):.1f});"))
            dpg.add_menu_item(label="Ресет", callback=lambda: add_to_code("reset();")); dpg.add_menu_item(label="Ресет X", callback=lambda: add_to_code("reset_x();")); dpg.add_menu_item(label="Ресет Y", callback=lambda: add_to_code("reset_y();"))

    with dpg.group(horizontal=True):
        with dpg.child_window(width=1000, border=False, tag="graph_container"):
            with dpg.plot(tag="plot_widget", height=-1, width=-1, equal_aspects=True, no_menus=True):
                x_ax, y_ax = dpg.add_plot_axis(dpg.mvXAxis, label="X"), dpg.add_plot_axis(dpg.mvYAxis, label="Y")
                dpg.add_scatter_series([], [], tag="scatter_series", parent=y_ax); dpg.add_line_series([], [], tag="line_series", parent=y_ax)
                dpg.add_line_series([], [], tag="lom_series", parent=y_ax); dpg.add_scatter_series([], [], tag="aux_scatter", parent=y_ax)
                dpg.add_line_series([], [], tag="aux_line", parent=y_ax); dpg.add_line_series([], [], tag="temp_lom_series", parent=y_ax)
                dpg.add_plot_annotation(tag="mouse_coord_tag", show=False); dpg.set_axis_limits(x_ax, 0, FIELD_W); dpg.set_axis_limits(y_ax, -FIELD_H, 0)
                dpg.set_axis_ticks(x_ax, tuple([(str(round(i*1.0, 1)), i*1.0) for i in range(int(FIELD_W)+2)])); dpg.set_axis_ticks(y_ax, tuple([(str(round(i*1.0, 1)), -i*1.0) for i in range(int(FIELD_H)+2)]))

        with dpg.child_window(width=-1, border=True):
            dpg.add_checkbox(label="Вспомогательная (//)", tag="aux_mode_cb"); dpg.add_checkbox(label="Сетка 1.0", tag="grid_snap_cb", default_value=True)
            dpg.add_input_int(label="Скорость", tag="speed_input", default_value=5000, step=500, step_fast=1000)
            dpg.add_button(label="Копировать код", callback=lambda: dpg.set_clipboard_text(dpg.get_value("side_code_output")), width=-1)
            dpg.add_input_text(tag="side_code_output", multiline=True, width=-1, height=-1, callback=lambda: (save_history(), sync_code_to_graph()))

dpg.bind_item_theme("scatter_series", main_theme); dpg.bind_item_theme("line_series", main_theme); dpg.bind_item_theme("lom_series", main_theme)
dpg.bind_item_theme("aux_scatter", aux_theme); dpg.bind_item_theme("aux_line", aux_theme); dpg.bind_item_theme("temp_lom_series", temp_theme)

with dpg.handler_registry():
    dpg.add_mouse_release_handler(callback=plot_callback)
    dpg.add_mouse_move_handler(callback=mouse_move_callback)
    
    def is_input_focused():
        return dpg.is_item_focused("side_code_output") or dpg.is_item_focused("speed_input")

    def create_key_handler(st):
        return lambda: (not is_input_focused() and 
                        (globals().update(status=st, fl=0, flr=0, arc_step=0, current_path_pts=[]), 
                         dpg.set_value("temp_lom_series", [[], []])))

    dpg.add_key_release_handler(key=dpg.mvKey_1, callback=create_key_handler(1))
    dpg.add_key_release_handler(key=dpg.mvKey_2, callback=create_key_handler(2))
    dpg.add_key_release_handler(key=dpg.mvKey_3, callback=create_key_handler(3))
    dpg.add_key_release_handler(key=dpg.mvKey_4, callback=create_key_handler(4))
    dpg.add_key_release_handler(key=dpg.mvKey_5, callback=create_key_handler(5))
    dpg.add_key_release_handler(key=dpg.mvKey_7, callback=create_key_handler(7))
    dpg.add_key_release_handler(key=dpg.mvKey_6, callback=lambda: not is_input_focused() and open_poly_settings())
    dpg.add_key_release_handler(key=dpg.mvKey_F11, callback=lambda: dpg.toggle_viewport_fullscreen())
    dpg.add_key_release_handler(key=dpg.mvKey_Escape, callback=abort_drawing)

dpg.set_viewport_resize_callback(resize_handler); dpg.create_viewport(title='CAD Editor', width=1600, height=950)
dpg.setup_dearpygui(); dpg.show_viewport(); dpg.set_primary_window("PrimaryWindow", True); resize_handler()

while dpg.is_dearpygui_running():
    ctrl = dpg.is_key_down(dpg.mvKey_LControl) or dpg.is_key_down(dpg.mvKey_RControl)
    if ctrl:
        if dpg.is_key_pressed(dpg.mvKey_Z): undo()
        if dpg.is_key_pressed(dpg.mvKey_Y): redo()
    dpg.render_dearpygui_frame()
dpg.destroy_context()
