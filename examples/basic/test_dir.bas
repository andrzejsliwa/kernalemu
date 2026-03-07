; test_dir.bas
; Test 1: Load and display directory listing from drive 8
; Exercises: OPEN with "$", INPUT#, CLOSE

10 print "=== DIRECTORY LISTING TEST ==="
20 print
30 open 1,8,0,"$"
40 if st<>0 then print "ERROR OPENING DIR: ";st:goto 200
50 input#1,a$
60 rem skip the load address line (first two bytes consumed by INPUT#)
70 print "reading directory..."
80 print
90 open 15,8,15,""
100 input#15,en,em$,et,es
110 print "drive status: ";en;",";em$;",";et;",";es
120 close 15
130 rem Now do a proper directory read via LOAD trick
140 close 1
150 print
160 print "reloading dir as data..."
170 open 1,8,0,"$"
180 rem read and print raw BASIC lines
190 get#1,a$:if st=0 then 190
200 close 1
210 print
220 print "done."
230 end
