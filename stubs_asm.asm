.model large
.code
public __chkstk
__chkstk proc
    pop cx
    sub sp, ax
    push cx
    ret
__chkstk endp
end
