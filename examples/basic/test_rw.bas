; test_rw.bas
; Test 2: Write a sequential file then read it back
; Exercises: OPEN for write (sa=1), PRINT#, CLOSE, OPEN for read, INPUT#

10 print "=== FILE READ/WRITE TEST ==="
20 print
30 rem --- Write phase ---
40 print "writing testfile.seq ..."
50 open 1,8,1,"testfile.seq,s,w"
60 if st<>0 then print "open write error ";st:goto 900
70 print#1,"line one"
80 print#1,"line two"
90 print#1,"line three"
100 print#1,"the end"
110 close 1
120 print "write done."
130 print
140 rem --- Read phase ---
150 print "reading testfile.seq ..."
160 open 2,8,2,"testfile.seq,s,r"
170 if st<>0 then print "open read error ";st:goto 900
180 n=0
190 input#2,l$
200 if st and 64 then goto 250
210 n=n+1
220 print n;": ";l$
230 goto 190
240 rem last line
250 n=n+1
260 print n;": ";l$
270 close 2
280 print
290 print "read done. lines=";n
300 goto 950
900 print "FAILED st=";st
910 open 15,8,15,""
920 input#15,en,em$,et,es
930 print "drive: ";en;em$
940 close 15
950 end
