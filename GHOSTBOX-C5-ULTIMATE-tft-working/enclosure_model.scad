// ==============================================================================
// GHOSTBOX C5 ULTIMATE - TACTICAL MIL-SPEC SCI-FI ENCLOSURE (OpenSCAD)
// Box Dimensions: 15.5 cm (L) x 10.5 cm (W) x 6.0 cm (H)
// Display: Standard 3.5" ILI9488 (Bezel + Screen: 75.0 x 51.5 mm active / 85x56 PCB)
// Design: Cyber-Skull / Cyberpunk Hacker Insignia Embossed
// ==============================================================================
// Instructions:
// 1. Open in OpenSCAD (https://openscad.org)
// 2. Press F5 for visual preview, F6 to compile & render.
// 3. File -> Export as STL (3D Print ready).
// ==============================================================================

$fn = 50;

// ==========================================
// PARAMETRIC DIMENSIONS
// ==========================================
wall_thick     = 2.6;
box_width      = 155.0;  // 15.5 cm Length
box_depth      = 105.0;  // 10.5 cm Depth
box_height     = 56.5;   // Base chassis depth
corner_radius  = 6.0;
screw_post_d   = 8.0;
screw_hole_d   = 2.8;    // Fits M3 / Self-tapping thread

// TFT Display - ILI9488 3.5" (480x320) exact viewport & recessed bezel
disp_w = 76.0;           // Active glass width (Landscape)
disp_h = 52.0;           // Active glass height (Landscape)
disp_x = 24.0;
disp_y = 26.5;

// Control Stack (Ergonomic Right-Hand Placement)
enc_d  = 7.5;            // EC11 Rotary Encoder shaft
enc_x  = 132.0;
enc_y  = 76.0;

btn_d  = 6.2;            // 6mm Tactical Tactile Switches
btn_x  = 132.0;
btn_y1 = 53.0;           // OK / SELECT
btn_y2 = 37.0;           // BACK / CANCEL
btn_y3 = 21.0;           // EXTRA / TACTICAL MODE

// Top SMA Antennas (CC1101 + NRF24 + GPS + SIM800L Cellular)
sma_d = 6.5;

// ==========================================
// UTILITY SHAPES & ARTWORK
// ==========================================
module rounded_cube(x, y, z, r) {
    translate([r, r, 0])
    minkowski() {
        cube([x - 2*r, y - 2*r, z/2]);
        cylinder(r=r, h=z/2);
    }
}

// 💀 Parametric Cyber-Skull Hacker Insignia (Embossed badge)
module cyber_skull(scale_factor=1.0) {
    scale([scale_factor, scale_factor, 1]) {
        difference() {
            union() {
                // Cranium / Head
                translate([0, 4, 0])
                    circle(r=10);
                // Jaw / Teeth Base
                translate([-5.5, -7, 0])
                    square([11, 8]);
                // Cyber Cheekbone Flares
                translate([-8.5, -1, 0])
                    polygon(points=[[0,0], [-2.5, 4], [0, 5]]);
                translate([8.5, -1, 0])
                    polygon(points=[[0,0], [2.5, 4], [0, 5]]);
            }
            
            // Eye Sockets (Angular Tactical Eyes)
            translate([-4.2, 3.5, 0])
                rotate([0, 0, 15])
                    scale([1.2, 0.9]) circle(r=2.6);
            translate([4.2, 3.5, 0])
                rotate([0, 0, -15])
                    scale([1.2, 0.9]) circle(r=2.6);

            // Nose Cavity (Inverted Triangle)
            translate([0, -0.5, 0])
                polygon(points=[[-1.5, 1.5], [1.5, 1.5], [0, -1.8]]);

            // Cyber Teeth Slots
            for (t = [-3.6, -1.2, 1.2, 3.6]) {
                translate([t - 0.4, -7.5, 0])
                    square([0.8, 4.0]);
            }
        }
        
        // Cyber Crossbones / Circuit Flares behind Skull
        for (a = [45, -45]) {
            rotate([0, 0, a]) {
                translate([-14, -0.8, 0]) square([28, 1.6]);
                translate([-14, -2.5, 0]) square([3, 5]);
                translate([11, -2.5, 0]) square([3, 5]);
            }
        }
    }
}

// ==========================================
// 1. LOWER TACTICAL CHASSIS (Bottom Case)
// ==========================================
module ghostbox_bottom_enclosure() {
    difference() {
        // Outer Rugged Shell
        union() {
            rounded_cube(box_width, box_depth, box_height, corner_radius);
            
            // Side Armor Ribs (Sci-Fi Grips)
            for (i = [0:6]) {
                translate([-1.2, 22 + (i*9), 10])
                    cube([2.0, 5.0, box_height - 20]);
                translate([box_width - 0.8, 22 + (i*9), 10])
                    cube([2.0, 5.0, box_height - 20]);
            }
            
            // Corner Bumper Accents
            translate([0, 0, 0]) cube([10, 10, 8]);
            translate([box_width - 10, 0, 0]) cube([10, 10, 8]);
            translate([0, box_depth - 10, 0]) cube([10, 10, 8]);
            translate([box_width - 10, box_depth - 10, 0]) cube([10, 10, 8]);
        }
        
        // Internal Module Cavity
        translate([wall_thick, wall_thick, wall_thick])
            rounded_cube(box_width - 2*wall_thick, box_depth - 2*wall_thick, box_height + 2, corner_radius - 1);
        
        // ------------------------------------
        // REAR / TOP ANTENNA ARRAY (4x SMA Ports - Near Base)
        // ------------------------------------
        translate([28, box_depth + 1, 14.0])
            rotate([90, 0, 0]) cylinder(d=sma_d, h=wall_thick*3);
        translate([62, box_depth + 1, 14.0])
            rotate([90, 0, 0]) cylinder(d=sma_d, h=wall_thick*3);
        translate([96, box_depth + 1, 14.0])
            rotate([90, 0, 0]) cylinder(d=sma_d, h=wall_thick*3);
        translate([130, box_depth + 1, 14.0])
            rotate([90, 0, 0]) cylinder(d=sma_d, h=wall_thick*3);

        // ------------------------------------
        // LEFT PORTS
        // ------------------------------------
        // MicroSD Slot
        translate([-2, 38, 16])
            cube([wall_thick*3, 16, 4.0]);

        // USB-C Programming & Power Port
        translate([-2, 70, 16])
            cube([wall_thick*3, 13, 7.5]);

        // ------------------------------------
        // FRONT OPTICAL & SENSOR PORTS
        // ------------------------------------
        // TSOP1738 IR Receiver Window
        translate([34, -2, 22])
            rotate([-90, 0, 0]) cylinder(d=5.8, h=wall_thick*3);
            
        // KY-005 IR Emitter Window
        translate([48, -2, 22])
            rotate([-90, 0, 0]) cylinder(d=6.2, h=wall_thick*3);

        // Status NeoPixel Diffuser Port
        translate([122, -2, 22])
            rotate([-90, 0, 0]) cylinder(d=4.5, h=wall_thick*3);

        // ------------------------------------
        // TACTICAL VENTILATION SLOTS
        // ------------------------------------
        for (v = [0:4]) {
            translate([box_width - wall_thick - 1, 32 + (v * 9), 16])
                cube([wall_thick*3, 4, 20]);
        }
    }

    // Screw Mounting Corner Pillars
    difference() {
        union() {
            translate([corner_radius + 1, corner_radius + 1, 0]) cylinder(d=screw_post_d, h=box_height - 1);
            translate([box_width - corner_radius - 1, corner_radius + 1, 0]) cylinder(d=screw_post_d, h=box_height - 1);
            translate([corner_radius + 1, box_depth - corner_radius - 1, 0]) cylinder(d=screw_post_d, h=box_height - 1);
            translate([box_width - corner_radius - 1, box_depth - corner_radius - 1, 0]) cylinder(d=screw_post_d, h=box_height - 1);
        }
        translate([corner_radius + 1, corner_radius + 1, 6]) cylinder(d=screw_hole_d, h=box_height);
        translate([box_width - corner_radius - 1, corner_radius + 1, 6]) cylinder(d=screw_hole_d, h=box_height);
        translate([corner_radius + 1, box_depth - corner_radius - 1, 6]) cylinder(d=screw_hole_d, h=box_height);
        translate([box_width - corner_radius - 1, box_depth - corner_radius - 1, 6]) cylinder(d=screw_hole_d, h=box_height);
    }
}

// ==========================================
// 2. TOP FACEPLATE (Sci-Fi Ghostbox HUD Bezel + Skull Emboss)
// ==========================================
module ghostbox_top_faceplate() {
    translate([0, box_depth + 20, 0]) { // Positioned adjacent on bed
        difference() {
            // Main Faceplate Body with Layered Armor
            union() {
                rounded_cube(box_width, box_depth, wall_thick + 1.2, corner_radius);
                
                // Bevel / Recessed Display Hood Guard
                translate([disp_x - 3, disp_y - 3, wall_thick + 1.2])
                    difference() {
                        cube([disp_w + 6, disp_h + 6, 2.0]);
                        translate([1.5, 1.5, -0.5])
                            cube([disp_w + 3, disp_h + 3, 3.0]);
                    }

                // Interlocking Seal Lip
                translate([wall_thick + 0.35, wall_thick + 0.35, -2.8])
                    rounded_cube(box_width - 2*wall_thick - 0.7, box_depth - 2*wall_thick - 0.7, 2.8, corner_radius - 1.5);
                
                // 💀 EMBOSSED CYBER-SKULL EMBLEM (3D Badge on Faceplate)
                translate([12, 52.5, wall_thick + 1.2])
                    linear_extrude(height=1.2)
                        cyber_skull(scale_factor=0.68);
            }

            // ------------------------------------
            // 3.5" ILI9488 TFT CUTOUT
            // ------------------------------------
            translate([disp_x, disp_y, -5])
                cube([disp_w, disp_h, 15]);

            // ------------------------------------
            // ROTARY ENCODER & CONTROLS
            // ------------------------------------
            // Rotary Dial
            translate([enc_x, enc_y, -5])
                cylinder(d=enc_d, h=15);

            // Tactile Push Buttons (OK, BACK, EXTRA)
            translate([btn_x, btn_y1, -5]) cylinder(d=btn_d, h=15);
            translate([btn_x, btn_y2, -5]) cylinder(d=btn_d, h=15);
            translate([btn_x, btn_y3, -5]) cylinder(d=btn_d, h=15);

            // ------------------------------------
            // ENGRAVED SCI-FI DETAILING & LABELS
            // ------------------------------------
            // Ghostbox Branding Engraving
            translate([disp_x + 2, disp_y + disp_h + 4.5, wall_thick + 0.8])
                linear_extrude(height=1.0)
                    text("GHOSTBOX C5 ULTIMATE", size=4.2, font="Arial:style=Bold");

            // Button Indicators
            translate([btn_x - 18, btn_y1 - 1.5, wall_thick + 0.8])
                linear_extrude(height=1.0)
                    text("OK", size=3.0, font="Arial:style=Bold");
            translate([btn_x - 22, btn_y2 - 1.5, wall_thick + 0.8])
                linear_extrude(height=1.0)
                    text("BACK", size=3.0, font="Arial:style=Bold");
            translate([btn_x - 22, btn_y3 - 1.5, wall_thick + 0.8])
                linear_extrude(height=1.0)
                    text("MODE", size=3.0, font="Arial:style=Bold");

            // Corner Screw Mounting Holes (M3 Countersunk)
            translate([corner_radius + 1, corner_radius + 1, -5]) cylinder(d=screw_hole_d + 0.6, h=15);
            translate([box_width - corner_radius - 1, corner_radius + 1, -5]) cylinder(d=screw_hole_d + 0.6, h=15);
            translate([corner_radius + 1, box_depth - corner_radius - 1, -5]) cylinder(d=screw_hole_d + 0.6, h=15);
            translate([box_width - corner_radius - 1, box_depth - corner_radius - 1, -5]) cylinder(d=screw_hole_d + 0.6, h=15);
            
            // Hex / Tactical Diagonal Speaker Vents
            for (i = [0:5]) {
                translate([disp_x + 10 + (i*10), 12, -5])
                    rotate([0, 0, 45])
                        cube([4.5, 4.5, 15]);
            }
        }
    }
}

// ==========================================
// RENDER
// ==========================================
ghostbox_bottom_enclosure();
ghostbox_top_faceplate();
