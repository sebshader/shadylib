# canvas_fill - the canvas (patch background)
# gop_box - the GOP rectangle (when editing GOP patches)
# obj_box_text - text in an object box
# msg_box_text - text in a message box
# comment
# selected - selection
# obj_box_outline_broken - outline of "broken" object 
#                          (that failed to create)
# obj_box_outline
# msg_box_outline
# msg_box_fill - fill of a message box
# obj_box_fill - " " object box
# signal_cord - signal cord and outline of signal inlets
# msg_cord - message cord and outline of message inlets
# msg_iolet - message inlet/outlet fill
# signal_iolet - signal inlet/outlet fill
# graph_outline - outline of arrays and GOP patches in the parent patch
# graph_text - color of the names of GOP patches in the parent patch
# selection_rectangle - selection rectangle color in edit mode
#                       txt_highlight - color text is highlighted
# (in the "background") when selected
# array_name - garray names
# array_values - array elements
# atom_box_fill - fill of gatoms (number box, symbol box)
# atom_box_text - text of gatoms
# atom_box_label - label of gatoms
# atom_box_outline - outline of gatoms
# text_window_fill - [text] window background
# text_window_text - [text] window text
# text_window_highlight - like txt_highlight but for [text] window
# text_window_cursor - [text] window cursor
# pdwindow_fill - background of post window
# pdwindow_fatal_text - text for fatal errors
# pdwindow_fatal_highlight - highlight (background) for fatal errors
# pdwindow_error_text - text for errors
# pdwindow_post_text - text for posting
# pdwindow_debug_text - text for verbose logs
# helpbrowser_fill
# helpbrowser_text
# helpbrowser_highlight - like txt_highlight but for help browser

array set ::pd_colors {
canvas_fill "#FF8800"
gop_box "black"
obj_box_text "#00FF00"
msg_box_text "white"
comment "white"
selected "yellow"
obj_box_outline_broken "light blue"
obj_box_outline "#0B8100"
msg_box_outline "purple"
msg_box_fill "purple"
obj_box_fill "#0B8100"
signal_cord "blue"
msg_cord "black"
msg_iolet "white"
signal_iolet "blue"
graph_outline "blue"
selection_rectangle "light green"
txt_highlight "black"
graph_text "purple"
array_name "#0B8100"
array_values "purple"
atom_box_fill "black"
atom_box_text "cyan"
atom_box_label "black"
atom_box_outline "cyan"
text_window_fill "black"
text_window_text "white"
text_window_highlight "green"
text_window_cursor "green"
pdwindow_fill "#000"
pdwindow_fatal_text "#D00"
pdwindow_fatal_highlight "#FFE0E8"
pdwindow_error_text "#D00"
pdwindow_post_text "deep sky blue"
pdwindow_debug_text "green"
helpbrowser_fill "#000"
helpbrowser_text "deep sky blue"
helpbrowser_highlight "red"
canvas_text_cursor "deep sky blue"
}
set ::pd_colors(txt_highlight_front) "$::pd_colors(selected)"
set ::pd_colors(msg_iolet_border) "blue"
set ::pd_colors(signal_iolet_border) $::pd_colors(signal_cord)


#random colors for everything
#proc ::pdtk_canvas::get_color {type {window 0}} {
#	return [format #%06x [expr {int(rand() * 0xFFFFFF)}]]
#}

# proc redraw_cords {name, blank, op} {
# 	foreach wind [wm stackorder .] {
# 		if {[winfo class $wind] eq "PatchWindow"} {
# 			set canv ${wind}.c
# 			foreach record [$canv find withtag cord] {
# 				set tag [lindex [$canv gettags $record] 0]
# 				set coords [lreplace [$canv coords $tag] 2 end-2]
# 				::pdtk_canvas::pdtk_coords {*}$coords $tag $canv
# 			}
# 		}
# 	}	
# }
# 
# trace variable ::curve_cords w redraw_cords