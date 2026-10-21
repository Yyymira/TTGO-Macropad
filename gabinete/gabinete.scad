// =====================================================================
//  TTGO Macropad — gabinete imprimible en 3D
//  6 Cherry MX en grilla 3x2 (montaje en placa, pines al aire para soldar cables),
//  2 encoders EC11 a la izquierda de la TTGO T-Display, 2 pulsadores
//  que accionan los botones integrados de la placa, y pata trasera con
//  bisagra dentada (ángulo/altura regulable cada 15°).
//
//  Unidades: mm. Elegí qué pieza generar con `pieza`, o desde la CLI:
//    openscad -D 'pieza="tapa"' -o tapa.stl gabinete.scad
//  Las piezas sueltas salen ya orientadas para imprimir.
// =====================================================================

pieza = "ensamble"; // [ensamble, tapa, base, bisagra, pata, pulsador, prueba_ttgo, prueba_mx_enc]
angulo_pata = 30;   // solo para la vista "ensamble"

$fn = 48;
e = 0.01;

// ---------- TTGO T-Display: VERIFICAR CON CALIBRE ----------
pcb_l = 51.6;       // largo del PCB
pcb_w = 25.1;       // ancho del PCB
pcb_t = 1.6;        // espesor del PCB
stack_top = 3.2;    // cara superior del PCB -> vidrio del display
btn_h = 1.8;        // cara superior del PCB -> tope de los botones integrados
btn_dx = 22.2;      // botones: X desde el centro del PCB (positivo = lado USB-C)
btn_dy = 8.2;       // botones: ±Y desde el centro del PCB
disp_w = 25.0;      // área visible del display
disp_h = 14.9;
disp_dx = -5.65;    // centro del área visible respecto del centro del PCB (medido: la imagen empieza a 7.5 mm del borde opuesto al USB)
disp_dy = 0;
usb_z = 1.6;        // centro del USB-C por encima de la cara superior del PCB
usb_gap = 0.5;      // extremo del PCB (lado USB) -> cara interna de la pared
frame_extra_w = 1.0;   // holgura extra en el ancho del marco (medido en la prueba impresa)
side_btn_gap = 6;      // apertura en las paredes largas para el botón lateral (pedido: 4, agrandada tras la 2ª prueba)
side_btn_from_end = 10.5; // de esa apertura al extremo USB de la placa (pedido: 11)
usb_conn_l = 9.5;      // el conector USB-C de la placa se mete hasta acá desde el extremo (llega a los botones)
usb_conn_w = 9.6;      // ancho del rebaje para el conector (entra justo entre los pulsadores)

// ---------- Cherry MX ----------
mx_cut = 15.0;      // agujero cuadrado (medido en la prueba impresa; el estándar es 14)
mx_plate = 1.5;     // espesor donde enganchan las trabas del switch
mx_pocket = 17;     // vaciado inferior para que las trabas puedan flexar
mx_pitch = 19.05;

// ---------- EC11 ----------
enc_hole = 9.4;     // buje roscado M7 (+2 mm tras la 2ª prueba impresa)
enc_panel = 1.6;    // espesor de tapa en el encoder (entra arandela + tuerca)
enc_pocket = 14;    // vaciado inferior para el cuerpo del encoder

// ---------- Caja ----------
wall = 2.4;
floor_t = 2.4;
lid_t = 3.0;
inner_w = 90;
inner_d = 89;
inner_h = 15;       // espacio libre interno (switches + cables)
r_out = 4;
tol = 0.25;
lip_h = 2;
lip_t = 1.6;
pad_h = 4;          // refuerzos del piso para tornillos de la tapa
pad_h_b = 6;        // refuerzos del piso para la bisagra

// ---------- Layout (vista superior, Y+ = fondo) ----------
sw_y = -14.5;       // fila de atrás de switches (la de adelante va un pitch más cerca)
top_y = 23.5;
enc_x = -28;
enc_dy = 11;
ttgo_x = inner_w/2 - usb_gap - pcb_l/2;

H_b = floor_t + inner_h;  // altura de la base = cara inferior de la tapa
z_top = H_b + lid_t;
z_pcb_top = H_b - stack_top;
z_pcb_bot = z_pcb_top - pcb_t;
frame_h = stack_top + pcb_t + 1.4;

// orden = número de botón en el firmware: atrás 0-1-2, adelante 3-4-5
sw_pos  = [for (r = [0:1], c = [0:2]) [(c - 1) * mx_pitch, sw_y - r * mx_pitch]];
enc_pos = [[enc_x, top_y - enc_dy], [enc_x, top_y + enc_dy]];
btn_pos = [[ttgo_x + btn_dx, top_y - btn_dy], [ttgo_x + btn_dx, top_y + btn_dy]];
lid_screws = [for (sx = [-1, 1], sy = [-1, 1]) [sx * (inner_w/2 - 3.6), sy * (inner_d/2 - 3.6)]];

// ---------- Bisagra / pata ----------
y_h = 39.5;         // eje de la bisagra
br_t = 3;           // espesor de la placa de la bisagra
k_r = 5;            // radio de nudillos
z_ax = -br_t - k_r - 0.5;
bw = 12;            // ancho del barril de la pata
kw = 5;             // ancho de cada nudillo fijo
th = 0.6;           // altura de los dientes
n_teeth = 24;       // 360/24 = un "click" cada 15°
cl = 0.15;
br_screws = [[-16.5, y_h], [16.5, y_h]];
leg_l = 40;
leg_w = 50;
leg_t = 4;

// ---------- Pulsadores ----------
pl_shaft = 4.0;
pl_flange = 7.0;
pl_flange_t = 1.0;
pl_pocket_h = 1.4;
pl_protr = 1.5;     // cuánto sobresale por encima de la tapa

// =====================================================================
module rrect(w, d, r) { offset(r) square([w - 2*r, d - 2*r], center = true); }

module usb_tunnel() {
    zc = z_pcb_top + usb_z;
    translate([ttgo_x + pcb_l/2 - 0.5, top_y, zc])
        rotate([0, 90, 0]) linear_extrude(20) rrect(8, 13, 2);
}

// ---------------------------- BASE ----------------------------------
module base() {
    difference() {
        union() {
            difference() {
                linear_extrude(H_b) rrect(inner_w + 2*wall, inner_d + 2*wall, r_out);
                translate([0, 0, floor_t]) linear_extrude(H_b) rrect(inner_w, inner_d, r_out - wall);
            }
            for (p = lid_screws) translate([p.x, p.y, 0]) cylinder(d = 8, h = floor_t + pad_h);
            for (p = br_screws) translate([p.x, p.y, 0]) cylinder(d = 8, h = floor_t + pad_h_b);
        }
        for (p = lid_screws) translate([p.x, p.y, -e]) {
            cylinder(d = 3.4, h = 20);
            cylinder(d = 6.2, h = 2.6);
        }
        for (p = br_screws) translate([p.x, p.y, -e]) cylinder(d = 2.6, h = floor_t + pad_h_b - 0.8);
        // huecos para patitas de goma adhesivas (adelante)
        for (sx = [-1, 1]) translate([sx * (inner_w/2 - 6), -(inner_d/2 - 6), -e]) cylinder(d = 10.2, h = 0.6);
        usb_tunnel();
    }
}

// ---------------------------- TAPA ----------------------------------
module snap_lip(len) {
    // traba en cuña: entra a presión desde abajo y retiene el PCB
    rotate([90, 0, 90]) linear_extrude(len, center = true)
        polygon([[0, -0.1], [-0.6, -0.1], [0, -1.3]]);
}

module ttgo_frame() {
    x0 = ttgo_x - pcb_l/2 - 0.3 - 1.2;
    x1 = ttgo_x + pcb_l/2;
    yi = pcb_w/2 + 0.3 + frame_extra_w/2;
    translate([0, 0, H_b - frame_h]) difference() {
        union() {
            // paredes largas
            for (s = [-1, 1]) translate([x0, top_y + s * (yi + 0.6) - 0.6, 0]) cube([x1 - x0, 1.2, frame_h + e]);
            // pared del lado opuesto al USB
            translate([x0, top_y - yi - 1.2, 0]) cube([1.2, 2 * yi + 2.4, frame_h + e]);
        }
        // apertura para el botón lateral, en ambas paredes largas
        translate([x1 - side_btn_from_end - side_btn_gap, top_y - yi - 2, -1])
            cube([side_btn_gap, 2 * yi + 4, frame_h + 2]);
    }
    // las trabas quedan lejos de la apertura del botón lateral
    for (s = [-1, 1], dx = [-12, 2])
        translate([ttgo_x + dx, top_y + s * yi, z_pcb_bot])
            mirror([0, s < 0 ? 1 : 0, 0]) snap_lip(8);
}

module lid_lip() {
    translate([0, 0, H_b - lip_h]) difference() {
        linear_extrude(lip_h + e) difference() {
            rrect(inner_w - 2*tol, inner_d - 2*tol, r_out - wall);
            rrect(inner_w - 2*tol - 2*lip_t, inner_d - 2*tol - 2*lip_t, 0.5);
        }
        // sin labio donde está el extremo USB de la placa
        translate([inner_w/2 - 6, top_y - pcb_w/2 - 2, -1]) cube([10, pcb_w + 4, lip_h + 2]);
    }
}

module tapa() {
    difference() {
        union() {
            translate([0, 0, H_b]) linear_extrude(lid_t) rrect(inner_w + 2*wall, inner_d + 2*wall, r_out);
            lid_lip();
            for (p = lid_screws) translate([p.x, p.y, floor_t + pad_h + 0.3])
                cylinder(d = 6.4, h = H_b - (floor_t + pad_h + 0.3) + e);
            ttgo_frame();
        }
        // Cherry MX: agujero mx_cut x mx_cut + vaciado inferior dejando 1.5 mm de "placa"
        for (p = sw_pos) translate([p.x, p.y, 0]) {
            translate([-mx_cut/2, -mx_cut/2, H_b - 1]) cube([mx_cut, mx_cut, lid_t + 2]);
            translate([-mx_pocket/2, -mx_pocket/2, H_b - 1]) cube([mx_pocket, mx_pocket, lid_t - mx_plate + 1]);
        }
        // EC11
        for (p = enc_pos) translate([p.x, p.y, 0]) {
            translate([0, 0, H_b - 1]) cylinder(d = enc_hole, h = lid_t + 2);
            translate([-enc_pocket/2, -enc_pocket/2, H_b - 1]) cube([enc_pocket, enc_pocket, lid_t - enc_panel + 1]);
        }
        // ventana del display, biselada a 45°
        hull() {
            translate([ttgo_x + disp_dx, top_y + disp_dy, H_b - e])
                linear_extrude(e) square([disp_w + 1, disp_h + 1], center = true);
            translate([ttgo_x + disp_dx, top_y + disp_dy, z_top])
                linear_extrude(e) square([disp_w + 1 + 2*lid_t, disp_h + 1 + 2*lid_t], center = true);
        }
        // pulsadores de los botones integrados
        for (p = btn_pos) translate([p.x, p.y, 0]) {
            translate([0, 0, H_b - 1]) cylinder(d = pl_shaft + 0.5, h = lid_t + 2);
            translate([0, 0, H_b - e]) cylinder(d = pl_flange + 0.6, h = pl_pocket_h + e);
        }
        // agujeros para autorroscantes M3 en las torretas
        for (p = lid_screws) translate([p.x, p.y, floor_t + pad_h]) cylinder(d = 2.6, h = H_b - floor_t - pad_h + 1.5);
        usb_tunnel();
        // rebaje en la cara inferior para el cuerpo del conector USB-C, que entra hasta los botones
        translate([ttgo_x + pcb_l/2 - usb_conn_l, top_y - usb_conn_w/2, H_b - 1])
            cube([usb_conn_l + 1, usb_conn_w, 1 + pl_pocket_h]);
    }
}

// ------------------------- PULSADOR ---------------------------------
pl_nub = pl_pocket_h - pl_flange_t + stack_top - btn_h - 0.1;
pl_shaft_l = lid_t - pl_pocket_h + pl_protr;

module pulsador() {  // z=0 en la punta que toca el botón
    cylinder(d = 2.6, h = pl_nub + e);
    translate([0, 0, pl_nub]) cylinder(d = pl_flange, h = pl_flange_t + e);
    translate([0, 0, pl_nub + pl_flange_t]) cylinder(d = pl_shaft, h = pl_shaft_l);
}

// ---------------------- BISAGRA Y PATA ------------------------------
// Marco local de la bisagra: eje = z', x' = hacia el frente, y' = hacia abajo.
module hinge_xf() {
    translate([0, y_h, z_ax]) multmatrix([[0, 0, 1, 0], [-1, 0, 0, 0], [0, -1, 0, 0], [0, 0, 0, 1]]) children();
}

module teeth(r1 = 2.2, r2 = k_r, h = th, phase = 0) {
    a = 180 / n_teeth * 1.04;  // leve solape entre dientes vecinos: evita aristas compartidas
    for (i = [0:n_teeth - 1]) rotate([0, 0, phase + i * 360 / n_teeth])
        translate([0, 0, -0.05]) polyhedron(
            points = [[r1, -r1*tan(a), 0], [r2, -r2*tan(a), 0], [r2, r2*tan(a), 0], [r1, r1*tan(a), 0],
                      [r1, 0, h + e], [r2, 0, h + e]],
            faces = [[0, 1, 2, 3], [0, 4, 5, 1], [3, 2, 5, 4], [0, 3, 4], [1, 5, 2]]);
}

k0 = bw/2 + th + cl;  // cara interna de cada nudillo

module bisagra_local() {
    difference() {
        union() {
            for (s = [-1, 1]) mirror([0, 0, s < 0 ? 1 : 0]) {
                translate([0, 0, k0]) hull() {
                    cylinder(r = k_r, h = kw);
                    translate([-k_r, -(k_r + 0.6), 0]) cube([2*k_r, e, kw]);
                }
                translate([0, 0, k0]) mirror([0, 0, 1]) teeth(phase = 180 / n_teeth);
            }
        }
        translate([0, 0, -20]) cylinder(d = 3.4, h = 40);
        translate([0, 0, k0 + kw - 2.5]) cylinder(d = 6.4, h = 3, $fn = 6);   // traba de tuerca M3
        translate([0, 0, -k0 - kw - 1]) cylinder(d = 6, h = 2);                // asiento de cabeza
    }
}

module bisagra() {
    difference() {
        union() {
            translate([-20, y_h - 7, -br_t]) cube([40, 13, br_t]);
            hinge_xf() bisagra_local();
        }
        for (p = br_screws) translate([p.x, p.y, -br_t - e]) {
            cylinder(d = 3.4, h = br_t + 1);
            cylinder(d = 6.2, h = 1.5);
        }
    }
}

module pata_local() {
    difference() {
        union() {
            translate([0, 0, -bw/2]) cylinder(r = k_r, h = bw);
            for (s = [-1, 1]) mirror([0, 0, s < 0 ? 1 : 0]) translate([0, 0, bw/2]) teeth();
            // cuello angosto (pasa entre los nudillos) + abanico + pata
            hull() {
                translate([0, -k_r, -bw/2]) cube([9, leg_t, bw]);
            }
            hull() {
                translate([9, -k_r, -bw/2]) cube([e, leg_t, bw]);
                translate([13, -k_r, -leg_w/2]) cube([e, leg_t, leg_w]);
            }
            translate([13, -k_r, -leg_w/2]) cube([leg_l - 13, leg_t, leg_w]);
            translate([leg_l, -k_r + leg_t/2, -leg_w/2]) cylinder(d = leg_t, h = leg_w);
        }
        translate([0, 0, -20]) cylinder(d = 3.4, h = 40);
    }
}

module pata(ang = 0) { hinge_xf() rotate([0, 0, ang]) pata_local(); }

// --------------------------- PRUEBAS --------------------------------
module prueba_ttgo() {
    intersection() {
        tapa();
        translate([ttgo_x - pcb_l/2 - 4, top_y - pcb_w/2 - 4, 0]) cube([pcb_l + 12, pcb_w + 8, 50]);
    }
}

module prueba_mx_enc() {
    intersection() {
        tapa();
        translate([enc_x - 10, sw_pos[0].y - 10, 0])
            cube([sw_pos[0].x + 10 - (enc_x - 10), enc_pos[0].y + 9 - (sw_pos[0].y - 10), 50]);
    }
}

// ------------------------- ORIENTACIÓN ------------------------------
module para_imprimir_tapa() { rotate([180, 0, 0]) translate([0, 0, -z_top]) children(); }

module ghost_parts() {
    // TTGO
    color("#1565c0", 0.8) translate([ttgo_x - pcb_l/2, top_y - pcb_w/2, z_pcb_bot]) cube([pcb_l, pcb_w, pcb_t]);
    color("#111", 0.9) translate([ttgo_x + disp_dx - 15, top_y - 8.5, z_pcb_top]) cube([30, 17, stack_top]);
    // switches MX (cuerpo bajo placa + keycap aproximado)
    for (p = sw_pos) translate([p.x, p.y, 0]) {
        color("#795548") translate([-7, -7, z_top - mx_plate - 5]) cube([14, 14, 5]);
        color("#ddd", 0.6) translate([-9, -9, z_top + 6]) cube([18, 18, 8]);
    }
    // EC11 + perilla
    for (p = enc_pos) translate([p.x, p.y, 0]) {
        color("#9e9e9e") translate([-6, -6, z_top - enc_panel - 6.5]) cube([12, 12, 6.5]);
        color("#333", 0.8) translate([0, 0, z_top + 2]) cylinder(d = 20, h = 15);
    }
}

if (pieza == "ensamble") {
    color("#eceff1") base();
    color("#cfd8dc", 0.85) tapa();
    color("#ff7043") bisagra();
    color("#ffa726") pata(angulo_pata);
    for (p = btn_pos) color("#e53935") translate([p.x, p.y, H_b + pl_pocket_h - pl_flange_t - pl_nub]) pulsador();
    ghost_parts();
} else if (pieza == "tapa") {
    para_imprimir_tapa() tapa();
} else if (pieza == "base") {
    base();
} else if (pieza == "bisagra") {
    rotate([180, 0, 0]) translate([0, -y_h, 0]) bisagra();
} else if (pieza == "pata") {
    translate([0, 0, k_r]) rotate([90, 0, 0]) pata_local();
} else if (pieza == "pulsador") {
    for (i = [0, 1]) translate([i * 10, 0, pl_nub + pl_flange_t + pl_shaft_l]) rotate([180, 0, 0]) pulsador();
} else if (pieza == "prueba_ttgo") {
    para_imprimir_tapa() prueba_ttgo();
} else if (pieza == "prueba_mx_enc") {
    para_imprimir_tapa() prueba_mx_enc();
}
