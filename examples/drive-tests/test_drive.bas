10 rem *** 1541 dos test - basic v2 ***
20 pa=0:fa=0
30 print chr$(147)
40 print "1541 dos emulation test"
50 print "========================"
60 print ""
100 rem test 1: ui ident
110 open 15,8,15,"ui"
120 input#15,a,b$,c,d
130 close 15
140 if a=73 then goto 160
150 fa=fa+1:print "test 1 fail":goto 200
160 pa=pa+1:print "test 1 pass (ident)"
200 rem test 2: write and read file
210 open 2,8,1,"drtest"
220 print#2,"hello world"
230 close 2
240 open 3,8,0,"drtest"
250 input#3,r$
260 close 3
270 if r$="hello world" then goto 290
280 fa=fa+1:print "test 2 fail":goto 300
290 pa=pa+1:print "test 2 pass (write/read)"
300 rem test 3: scratch
310 open 15,8,15,"s:drtest"
320 input#15,a,b$,c,d
330 close 15
340 if a=0 then goto 370
350 if a=1 then goto 370
360 fa=fa+1:print "test 3 fail":goto 400
370 pa=pa+1:print "test 3 pass (scratch)"
400 rem test 4: rename
410 open 2,8,1,"rntest"
420 print#2,"x"
430 close 2
440 open 15,8,15,"r:rntest2=rntest"
450 input#15,a,b$,c,d
460 close 15
470 if a=0 then goto 490
480 fa=fa+1:print "test 4 fail":goto 500
490 pa=pa+1:print "test 4 pass (rename)"
500 rem test 5: copy
510 open 15,8,15,"c:rntest3=rntest2"
520 input#15,a,b$,c,d
530 close 15
540 if a=0 then goto 560
550 fa=fa+1:print "test 5 fail":goto 600
560 pa=pa+1:print "test 5 pass (copy)"
600 rem test 6: directory listing
610 open 8,8,0,"$"
620 if st<>0 then goto 650
630 close 8
640 pa=pa+1:print "test 6 pass (dir)":goto 700
650 close 8:fa=fa+1:print "test 6 fail"
700 rem test 7: file not found error (rename nonexistent)
710 open 15,8,15,"r:newname=nosuchfile"
720 input#15,a,b$,c,d
730 close 15
740 if a=62 then goto 780
750 fa=fa+1:print "test 7 fail (got ";a;")":goto 800
780 pa=pa+1:print "test 7 pass (error check)"
800 rem cleanup
810 open 15,8,15,"s:rntest2"
820 close 15
830 open 15,8,15,"s:rntest3"
840 close 15
900 rem summary
910 print ""
920 print "========================"
930 print "passed:";pa
940 print "failed:";fa
950 rem exit 0 if all passed, 1 if any failed
960 if fa=0 then sys 65280
970 sys 65281
