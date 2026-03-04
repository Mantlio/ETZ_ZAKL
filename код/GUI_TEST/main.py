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

def update_visibility():
    dpg.configure_item("manual_input_group", show=(status == 1))
    dpg.configure_item("line_input_group", show=(status == 2))
    dpg.configure_item("circle_input_group", show=(status == 4))
    dpg.configure_item("poly_input_group", show=(status == 6))
    dpg.configure_item("arc_input_group", show=(status == 7))

def get_snapped_pos():
    pos = dpg.get_plot_mouse_pos()
    snap = 1.0 if dpg.get_value("grid_snap_cb") else 0.1
    gx = round(pos[0] / snap) * snap
    gy = round(pos[1] / snap) * snap
    if status == 1:
        if dpg.get_value("lock_x"): gx = dpg.get_value("manual_x")
        if dpg.get_value("lock_y"): gy = -dpg.get_value("manual_y")
    elif status == 4:
        if dpg.get_value("lock_cx"): gx = dpg.get_value("circle_cx")
        if dpg.get_value("lock_cy"): gy = -dpg.get_value("circle_cy")
    elif status == 6:
        if dpg.get_value("lock_poly_cx"): gx = dpg.get_value("poly_cx")
        if dpg.get_value("lock_poly_cy"): gy = -dpg.get_value("poly_cy")
    elif status == 7:
        if dpg.get_value("lock_arc_cx"): gx = dpg.get_value("arc_cx")
        if dpg.get_value("lock_arc_cy"): gy = -dpg.get_value("arc_cy")

    return gx, gy


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
        elif status == 3: current_path_pts.append((gx, gy, round(dpg.get_value("speed_input")/500)*500))
        elif status == 4:
            is_rad = dpg.get_value("circle_unit") == "Радиус"
            is_fixed = dpg.get_value("lock_cr")
            if is_fixed:
                r = dpg.get_value("circle_val") if is_rad else dpg.get_value("circle_val") / 2
                add_to_code(f"draw_circle({gx:.3f}, {abs(gy):.3f}, {r:.3f});")
            else:
                if flr == 0: c_x, c_y, flr = gx, gy, 1
                else:
                    r = math.sqrt((gx-c_x)**2+(gy-c_y)**2)
                    add_to_code(f"draw_circle({c_x:.3f}, {abs(c_y):.3f}, {r:.3f});"); flr = 0
        elif status == 5:
            if fl == 0: sx, sy, fl = gx, gy, 1
            else: add_to_code(f"draw_rect({sx:.3f}, {abs(sy):.3f}, {gx:.3f}, {abs(gy):.3f});"); fl = 0
        elif status == 6: add_manual_polygon()
        elif status == 7:
            if arc_step == 0:
                c_x, c_y, arc_step = gx, gy, 1
            elif arc_step == 1:
                sx, sy, arc_step = gx, gy, 2
            else:
                r = math.sqrt((sx-c_x)**2 + (sy-c_y)**2)
                a1 = math.atan2(sy-c_y, sx-c_x)
                a2 = math.atan2(gy-c_y, gx-c_x)
                if a2 < a1: a2 += 2*math.pi
                
                diff = a2 - a1
                segs = max(20, int(r * diff * 30))
                pts = []
                for i in range(segs + 1):
                    a = a1 + diff * i / segs
                    pts.append(f"{{ {c_x + r*math.cos(a):.3f}, {abs(c_y + r*math.sin(a)):.3f}, 0 }}")
                
                idx = dpg.get_value("side_code_output").count("Point arc") + 1
                add_to_code(f"Point arc{idx}[] = {{ {', '.join(pts)} }};\ndraw_path(arc{idx}, 0);")
                arc_step = 0
                dpg.set_value("temp_lom_series", [[], []])

def mouse_move_callback():
    global status, fl, sx, sy, flr, c_x, c_y, arc_step
    if dpg.is_item_hovered("plot_widget"):
        gx, gy = get_snapped_pos(); agy = abs(gy)
        if status == 1:
            if not dpg.get_value("lock_x"): dpg.set_value("manual_x", gx)
            if not dpg.get_value("lock_y"): dpg.set_value("manual_y", agy)
        elif status == 2:
            if fl == 0: dpg.set_value("l_m_x1", gx); dpg.set_value("l_m_y1", agy)
            else: dpg.set_value("l_m_x2", gx); dpg.set_value("l_m_y2", agy)
            if fl == 1: dpg.set_value("temp_lom_series", [[sx, gx], [sy, gy]])
        elif status == 4:
            if not dpg.get_value("lock_cx"): dpg.set_value("circle_cx", gx)
            if not dpg.get_value("lock_cy"): dpg.set_value("circle_cy", agy)
            is_fixed, is_rad = dpg.get_value("lock_cr"), dpg.get_value("circle_unit") == "Радиус"
            val = dpg.get_value("circle_val")
            if is_fixed:
                r = val if is_rad else val / 2
                tx = [gx + r * math.cos(2*math.pi*i/60) for i in range(61)]
                ty = [gy + r * math.sin(2*math.pi*i/60) for i in range(61)]
                dpg.set_value("temp_lom_series", [tx, ty])
            elif flr == 1:
                r = math.sqrt((gx-c_x)**2+(gy-c_y)**2)
                tx = [c_x + r * math.cos(2*math.pi*i/60) for i in range(61)]
                ty = [c_y + r * math.sin(2*math.pi*i/60) for i in range(61)]
                dpg.set_value("temp_lom_series", [tx, ty])
            else: dpg.set_value("temp_lom_series", [[], []])
        elif status == 6:
            if not dpg.get_value("lock_poly_cx"): dpg.set_value("poly_cx", gx)
            if not dpg.get_value("lock_poly_cy"): dpg.set_value("poly_cy", agy)
            
            cx = dpg.get_value("poly_cx")
            cy = -dpg.get_value("poly_cy") 
            
            n = max(3, dpg.get_value("poly_sides_val"))
            mode, val = dpg.get_value("poly_size_mode"), dpg.get_value("poly_size_val")

            rot = -(math.radians(dpg.get_value("poly_rotation")) + math.pi)

            
            r = val if mode == "Радиус" else (val/2 if mode == "Диаметр" else val/(2*math.sin(math.pi/n)))
            
            tx = [cx + r * math.cos(rot + 2*math.pi*i/n) for i in range(n + 1)]
            ty = [cy + r * math.sin(rot + 2*math.pi*i/n) for i in range(n + 1)]
            dpg.set_value("temp_lom_series", [tx, ty])

        elif status == 5:
            if fl == 1: dpg.set_value("temp_lom_series", [[sx, gx, gx, sx, sx], [sy, sy, gy, gy, sy]])
        elif status == 3 and len(current_path_pts) > 0:
            tx = [p[0] for p in current_path_pts] + [gx]; ty = [p[1] for p in current_path_pts] + [gy]
            dpg.set_value("temp_lom_series", [tx, ty])
        elif status == 7:
            if not dpg.get_value("lock_arc_cx"): dpg.set_value("arc_cx", gx)
            if not dpg.get_value("lock_arc_cy"): dpg.set_value("arc_cy", agy)
            
            if arc_step == 1:
                r = math.sqrt((gx-c_x)**2 + (gy-c_y)**2)
                dpg.set_value("arc_val", r if dpg.get_value("arc_unit") == "Радиус" else r*2)
                tx = [c_x + r * math.cos(2*math.pi*i/60) for i in range(61)]
                ty = [c_y + r * math.sin(2*math.pi*i/60) for i in range(61)]
                dpg.set_value("temp_lom_series", [tx, ty])
            elif arc_step == 2:
                r = math.sqrt((sx-c_x)**2 + (sy-c_y)**2)
                a1_rad = math.atan2(sy-c_y, sx-c_x)
                a2_rad = math.atan2(gy-c_y, gx-c_x)
                
                dpg.set_value("arc_a1", math.degrees(a1_rad) % 360)
                dpg.set_value("arc_a2", math.degrees(a2_rad) % 360)
                
                if a2_rad < a1_rad: a2_rad += 2*math.pi
                diff = a2_rad - a1_rad
                segs = max(15, int(r * diff * 20))
                tx = [c_x + r * math.cos(a1_rad + diff*i/segs) for i in range(segs+1)]
                ty = [c_y + r * math.sin(a1_rad + diff*i/segs) for i in range(segs+1)]
                dpg.set_value("temp_lom_series", [tx, ty])

        dpg.configure_item("mouse_coord_tag", label=f"{gx:.1f}, {agy:.1f}", default_value=[gx, gy], show=True)

    else: dpg.configure_item("mouse_coord_tag", show=False)

def add_manual_polygon():
    cx, cy = dpg.get_value("poly_cx"), dpg.get_value("poly_cy")
    n = max(3, dpg.get_value("poly_sides_val"))
    mode, val = dpg.get_value("poly_size_mode"), dpg.get_value("poly_size_val")
    
    rot = math.radians(dpg.get_value("poly_rotation")) + math.pi

    
    r = val if mode == "Радиус" else (val/2 if mode == "Диаметр" else val/(2*math.sin(math.pi/n)))
    
    pts = [f"{{ {cx + r * math.cos(rot + 2*math.pi*i/n):.3f}, {cy + r * math.sin(rot + 2*math.pi*i/n):.3f}, 0 }}" for i in range(n + 1)]
    
    idx = dpg.get_value('side_code_output').count('draw_path') + 1
    add_to_code(f"Point poly{idx}[] = {{ {', '.join(pts)} }};\ndraw_path(poly{idx}, 0);")

def add_manual_arc():
    cx, cy = dpg.get_value("arc_cx"), dpg.get_value("arc_cy")
    val = dpg.get_value("arc_val")
    r = val if dpg.get_value("arc_unit") == "Радиус" else val/2
    a1_deg, a2_deg = dpg.get_value("arc_a1"), dpg.get_value("arc_a2")
    
    a1, a2 = math.radians(a1_deg), math.radians(a2_deg)
    if a2 < a1: a2 += 2*math.pi
    
    diff = a2 - a1
    segs = max(20, int(r * diff * 30))
    pts = [f"{{ {cx + r*math.cos(a1 + diff*i/segs):.3f}, {cy + r*math.sin(a1 + diff*i/segs):.3f}, 0 }}" for i in range(segs+1)]
    
    idx = dpg.get_value('side_code_output').count('Point arc') + 1
    add_to_code(f"Point arc{idx}[] = {{ {', '.join(pts)} }};\ndraw_path(arc{idx}, 0);")


def abort_drawing():
    global fl, flr, arc_step; fl = flr = arc_step = 0
    current_path_pts.clear(); dpg.set_value("temp_lom_series", [[], []])

def resize_handler():
    h = dpg.get_viewport_height() - 80
    dpg.configure_item("graph_container", width=int(h * RATIO))

def is_input_focused():
    f = dpg.get_focused_item(); return "Input" in (dpg.get_item_info(f).get("type", "") if f else "")

def add_manual_point():
    mx, my = dpg.get_value("manual_x"), dpg.get_value("manual_y"); add_to_code(f"draw_point({mx:.3f}, {my:.3f});")

def add_manual_line():
    mode = dpg.get_value("line_manual_mode")
    x1, y1 = dpg.get_value("l_m_x1"), dpg.get_value("l_m_y1")
    if mode == "По точкам":
        x2, y2 = dpg.get_value("l_m_x2"), dpg.get_value("l_m_y2")
    else:
        leng, ang = dpg.get_value("l_m_len"), math.radians(dpg.get_value("l_m_ang"))
        x2, y2 = x1 + leng * math.cos(ang), y1 + leng * math.sin(ang)
    add_to_code(f"draw_line({x1:.3f}, {y1:.3f}, {x2:.3f}, {y2:.3f});")

def add_manual_circle():
    cx, cy, val = dpg.get_value("circle_cx"), dpg.get_value("circle_cy"), dpg.get_value("circle_val")
    r = val if dpg.get_value("circle_unit") == "Радиус" else val/2
    add_to_code(f"draw_circle({cx:.3f}, {cy:.3f}, {r:.3f});")

with dpg.theme() as main_theme:
    with dpg.theme_component(dpg.mvLineSeries): dpg.add_theme_color(dpg.mvPlotCol_Line, (0, 200, 255, 255), category=dpg.mvThemeCat_Plots)
with dpg.theme() as aux_theme:
    with dpg.theme_component(dpg.mvLineSeries): dpg.add_theme_color(dpg.mvPlotCol_Line, (180, 100, 255, 255), category=dpg.mvThemeCat_Plots)
with dpg.theme() as temp_theme:
    with dpg.theme_component(dpg.mvLineSeries): dpg.add_theme_color(dpg.mvPlotCol_Line, (255, 255, 255, 150), category=dpg.mvThemeCat_Plots)

with dpg.window(tag="PrimaryWindow"):
    with dpg.menu_bar():
        with dpg.menu(label="Файл"):
            dpg.add_menu_item(label="Новый", callback=lambda: (dpg.set_value("side_code_output", ""), save_history(), sync_code_to_graph()))
            dpg.add_menu_item(label="Во весь экран", callback=lambda: dpg.toggle_viewport_fullscreen())
            dpg.add_menu_item(label="Закрыть", callback=lambda: dpg.stop_dearpygui())
        with dpg.menu(label="Геометрия"):
            dpg.add_menu_item(label="Точка (P)", callback=lambda: (globals().update(status=1), update_visibility()))
            dpg.add_menu_item(label="Линия (L)", callback=lambda: (globals().update(status=2, fl=0), update_visibility()))
            dpg.add_menu_item(label="Ломанная (W)", callback=lambda: (globals().update(status=3, current_path_pts=[]), update_visibility()))
            dpg.add_menu_item(label="Круг (C)", callback=lambda: (globals().update(status=4, flr=0), update_visibility()))
            dpg.add_menu_item(label="Прямоугольник (R)", callback=lambda: (globals().update(status=5, fl=0), update_visibility()))
            dpg.add_menu_item(label="Многоугольник (G)", callback=lambda: (globals().update(status=6), update_visibility()))
            dpg.add_menu_item(label="Дуга (A)", callback=lambda: (globals().update(status=7, arc_step=0), update_visibility()))

    with dpg.group(horizontal=True):
        with dpg.child_window(width=1000, border=False, tag="graph_container"):
            with dpg.plot(tag="plot_widget", height=-1, width=-1, equal_aspects=True, no_menus=True):
                x_ax, y_ax = dpg.add_plot_axis(dpg.mvXAxis, label="X"), dpg.add_plot_axis(dpg.mvYAxis, label="Y")
                dpg.add_scatter_series([], [], tag="scatter_series", parent=y_ax)
                dpg.add_line_series([], [], tag="line_series", parent=y_ax)
                dpg.add_line_series([], [], tag="lom_series", parent=y_ax)
                dpg.add_scatter_series([], [], tag="aux_scatter", parent=y_ax)
                dpg.add_line_series([], [], tag="aux_line", parent=y_ax)
                dpg.add_line_series([], [], tag="temp_lom_series", parent=y_ax)
                dpg.add_plot_annotation(tag="mouse_coord_tag", show=False)
                dpg.set_axis_limits(x_ax, 0, FIELD_W); dpg.set_axis_limits(y_ax, -FIELD_H, 0)
                dpg.set_axis_ticks(x_ax, tuple([(str(round(i*1.0, 1)), i*1.0) for i in range(int(FIELD_W)+2)]))
                dpg.set_axis_ticks(y_ax, tuple([(str(round(i*1.0, 1)), -i*1.0) for i in range(int(FIELD_H)+2)]))

        with dpg.child_window(width=-1, border=True, tag="side_panel"):
            dpg.add_checkbox(label="Вспомогательная (//)", tag="aux_mode_cb")
            dpg.add_checkbox(label="Сетка 1.0", tag="grid_snap_cb", default_value=True)
            dpg.add_input_int(label="Скорость", tag="speed_input", default_value=5000, step=500, width=-1)
            
            with dpg.group(tag="manual_input_group", show=False):
                dpg.add_separator(); dpg.add_text("Параметры точки")
                with dpg.group(horizontal=True): dpg.add_checkbox(tag="lock_x", label="X"); dpg.add_input_float(tag="manual_x", width=150)
                with dpg.group(horizontal=True): dpg.add_checkbox(tag="lock_y", label="Y"); dpg.add_input_float(tag="manual_y", width=150)
                dpg.add_button(label="Добавить точку", callback=add_manual_point, width=-1)

            with dpg.group(tag="line_input_group", show=False):
                dpg.add_separator(); dpg.add_text("Линия")
                dpg.add_radio_button(["По точкам", "Длина + Угол"], tag="line_manual_mode", horizontal=True, default_value="По точкам")
                with dpg.group(horizontal=True): dpg.add_text("X1:"); dpg.add_input_float(tag="l_m_x1", width=110); dpg.add_text("Y1:"); dpg.add_input_float(tag="l_m_y1", width=110)
                with dpg.group(tag="line_mode_pts"):
                    with dpg.group(horizontal=True): dpg.add_text("X2:"); dpg.add_input_float(tag="l_m_x2", width=110); dpg.add_text("Y2:"); dpg.add_input_float(tag="l_m_y2", width=110)
                with dpg.group(tag="line_mode_ang", show=False):
                    with dpg.group(horizontal=True): dpg.add_text("L:"); dpg.add_input_float(tag="l_m_len", width=110); dpg.add_text("A:"); dpg.add_input_float(tag="l_m_ang", width=110)
                dpg.configure_item("line_manual_mode", callback=lambda s,v: (dpg.configure_item("line_mode_pts", show=v=="По точкам"), dpg.configure_item("line_mode_ang", show=v!="По точкам")))
                dpg.add_button(label="Добавить линию", callback=add_manual_line, width=-1)

            with dpg.group(tag="circle_input_group", show=False):
                dpg.add_separator(); dpg.add_text("Круг")
                with dpg.group(horizontal=True): dpg.add_checkbox(tag="lock_cx", label="CX"); dpg.add_input_float(tag="circle_cx", width=150)
                with dpg.group(horizontal=True): dpg.add_checkbox(tag="lock_cy", label="CY"); dpg.add_input_float(tag="circle_cy", width=150)
                dpg.add_combo(["Радиус", "Диаметр"], tag="circle_unit", default_value="Радиус")
                with dpg.group(horizontal=True): dpg.add_checkbox(tag="lock_cr", label="Фикс"); dpg.add_input_float(tag="circle_val", width=150)
                dpg.add_button(label="Добавить круг", callback=add_manual_circle, width=-1)

            with dpg.group(tag="poly_input_group", show=False):
                dpg.add_separator(); dpg.add_text("Многоугольник")
                with dpg.group(horizontal=True): dpg.add_checkbox(tag="lock_poly_cx", label="CX"); dpg.add_input_float(tag="poly_cx", width=150)
                with dpg.group(horizontal=True): dpg.add_checkbox(tag="lock_poly_cy", label="CY"); dpg.add_input_float(tag="poly_cy", width=150)
                dpg.add_input_int(label="Углы", tag="poly_sides_val", default_value=6)
                dpg.add_combo(["Радиус", "Диаметр", "Длина стороны"], tag="poly_size_mode", default_value="Радиус")
                dpg.add_input_float(label="Знач", tag="poly_size_val", default_value=2.0)
                dpg.add_input_float(label="Поворот", tag="poly_rotation", default_value=0.0)
                dpg.add_button(label="Добавить", callback=add_manual_polygon, width=-1, height=40)

            with dpg.group(tag="arc_input_group", show=False):
                dpg.add_separator(); dpg.add_text("Параметры Дуги")
                with dpg.group(horizontal=True): 
                    dpg.add_checkbox(tag="lock_arc_cx", label="CX"); dpg.add_input_float(tag="arc_cx", width=150)
                with dpg.group(horizontal=True): 
                    dpg.add_checkbox(tag="lock_arc_cy", label="CY"); dpg.add_input_float(tag="arc_cy", width=150)
                
                dpg.add_combo(["Радиус", "Диаметр"], tag="arc_unit", default_value="Радиус")
                dpg.add_input_float(label="Знач", tag="arc_val", default_value=5.0)
                
                with dpg.group(horizontal=True):
                    dpg.add_text("Углы:"); dpg.add_input_float(tag="arc_a1", width=100, label="Нач"); dpg.add_input_float(tag="arc_a2", width=100, label="Кон")
                
                dpg.add_button(label="Добавить дугу", callback=lambda: add_manual_arc(), width=-1, height=40)


            dpg.add_separator(); dpg.add_input_text(tag="side_code_output", multiline=True, width=-1, height=-1, callback=lambda: (save_history(), sync_code_to_graph()))

dpg.bind_item_theme("scatter_series", main_theme); dpg.bind_item_theme("line_series", main_theme); dpg.bind_item_theme("lom_series", main_theme); dpg.bind_item_theme("temp_lom_series", temp_theme)

with dpg.handler_registry():
    dpg.add_mouse_release_handler(callback=plot_callback)
    dpg.add_mouse_move_handler(callback=mouse_move_callback)
    def key_cb(st):
        def _c():
            if not is_input_focused(): abort_drawing(); globals().update(status=st); update_visibility()
        return _c
    dpg.add_key_release_handler(key=dpg.mvKey_P, callback=key_cb(1))
    dpg.add_key_release_handler(key=dpg.mvKey_L, callback=key_cb(2))
    dpg.add_key_release_handler(key=dpg.mvKey_W, callback=key_cb(3))
    dpg.add_key_release_handler(key=dpg.mvKey_C, callback=key_cb(4))
    dpg.add_key_release_handler(key=dpg.mvKey_R, callback=key_cb(5))
    dpg.add_key_release_handler(key=dpg.mvKey_G, callback=key_cb(6))
    dpg.add_key_release_handler(key=dpg.mvKey_A, callback=key_cb(7))
    dpg.add_key_release_handler(key=dpg.mvKey_Escape, callback=abort_drawing)
    def handle_ur(s, d):
        if not is_input_focused() and (dpg.is_key_down(dpg.mvKey_LControl) or dpg.is_key_down(dpg.mvKey_RControl)):
            if d == dpg.mvKey_Z: undo()
            if d == dpg.mvKey_Y: redo()
    dpg.add_key_release_handler(callback=handle_ur)

dpg.create_viewport(title='CAD Editor Pro', width=1600, height=900)
dpg.setup_dearpygui()
dpg.show_viewport()
dpg.set_viewport_resize_callback(resize_handler)
dpg.set_primary_window("PrimaryWindow", True)

resize_handler()
sync_code_to_graph()

dpg.start_dearpygui()
dpg.destroy_context()
