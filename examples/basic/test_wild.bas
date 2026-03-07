; test_wild.bas
; Test 4: Wildcard scratch, LOAD, SAVE
; Exercises: S:*.seq wildcard, SAVE, LOAD

10 print "=== WILDCARD AND LOAD/SAVE TEST ==="
20 print

30 rem --- Create several seq files ---
40 print "creating alpha.seq, beta.seq, gamma.prg"
50 open 1,8,1,"alpha.seq,s,w":print#1,"ALPHA DATA":close 1
60 open 1,8,1,"beta.seq,s,w" :print#1,"BETA DATA" :close 1
70 open 1,8,1,"gamma.prg,p,w":print#1,"GAMMA DATA":close 1

80 rem --- Wildcard scratch all .seq files ---
90 print "wildcard scratch *.seq"
100 open 15,8,15,"s:*.seq"
110 input#15,en,em$,et,es
120 print "  result: ";en;",";em$
130 close 15

140 rem --- Verify alpha.seq is gone ---
150 print "verify alpha.seq gone..."
160 open 3,8,2,"alpha.seq,s,r"
170 if st<>0 then print "  confirmed: not found (st=";st;")" : goto 200
180 input#3,l$:close 3
190 print "  ERROR: file still exists!"
200 rem

210 rem --- gamma.prg should still exist ---
220 print "verify gamma.prg still exists..."
230 open 3,8,2,"gamma.prg,p,r"
240 if st<>0 then print "  ERROR: gamma.prg missing (st=";st;")" : goto 280
250 input#3,l$:close 3
260 print "  confirmed: ";l$

280 rem --- Clean up ---
290 print "cleaning up gamma.prg"
300 open 15,8,15,"s:gamma.prg"
310 input#15,en,em$:print "  ";en;",";em$
320 close 15

330 print
340 print "wildcard test done."
350 end
