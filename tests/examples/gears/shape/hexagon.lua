--DOC_GEN_IMAGE --DOC_HIDE
local shape,cr,show = ... --DOC_HIDE

shape.hexagon(cr, 70, 70)
show(cr) --DOC_HIDE

shape.transform(shape.hexagon) : translate(0,15)(cr,70,20)
show(cr) --DOC_HIDE

shape.transform(shape.hexagon) : rotate_at(35,35,math.pi/2)(cr,70,40)
show(cr) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
