; test_dos.bas
; Test 3: DOS command channel - scratch, rename, copy, status
; Exercises: OPEN sa=15 (command channel), PRINT# for commands,
;            INPUT# for status, S: R: C: I UI V

10 print "=== DOS COMMAND CHANNEL TEST ==="
20 print

30 rem --- Helper: open cmd channel, send command, read status ---
40 rem (we re-open each time to keep it simple)

50 rem --- Step 1: create a file to work with ---
60 print "step 1: create work.seq"
70 open 1,8,1,"work.seq,s,w"
80 print#1,"hello from basic"
90 close 1

100 rem --- Step 2: check drive status (idle read) ---
110 print "step 2: drive status after create"
120 open 15,8,15,""
130 input#15,en,em$,et,es
140 print "  ";en;",";em$;",";et;",";es
150 close 15

160 rem --- Step 3: rename work.seq -> renamed.seq ---
170 print "step 3: rename work.seq -> renamed.seq"
180 open 15,8,15,"r:renamed.seq=work.seq"
190 input#15,en,em$,et,es
200 print "  ";en;",";em$;",";et;",";es
210 close 15

220 rem --- Step 4: copy renamed.seq -> copy.seq ---
230 print "step 4: copy renamed.seq -> copy.seq"
240 open 15,8,15,"c:copy.seq=renamed.seq"
250 input#15,en,em$,et,es
260 print "  ";en;",";em$;",";et;",";es
270 close 15

280 rem --- Step 5: verify copy exists by reading it ---
290 print "step 5: read copy.seq"
300 open 2,8,2,"copy.seq,s,r"
310 if st<>0 then print "  could not open copy.seq":goto 400
320 input#2,l$
330 print "  got: ";l$
340 close 2

350 rem --- Step 6: validate (V command) ---
360 print "step 6: validate"
370 open 15,8,15,"v"
380 input#15,en,em$,et,es
390 print "  ";en;",";em$;",";et;",";es
400 close 15

410 rem --- Step 7: scratch both files ---
420 print "step 7: scratch renamed.seq,copy.seq"
430 open 15,8,15,"s:renamed.seq,copy.seq"
440 input#15,en,em$,et,es
450 print "  ";en;",";em$;",";et;",";es
460 close 15

470 rem --- Step 8: soft reset, check ident ---
480 print "step 8: soft reset UI"
490 open 15,8,15,"ui"
500 input#15,en,em$,et,es
510 print "  ";en;",";em$;",";et;",";es
520 close 15

530 rem --- Step 9: try scratching nonexistent file ---
540 print "step 9: scratch nonexistent"
550 open 15,8,15,"s:nosuchfile.seq"
560 input#15,en,em$,et,es
570 print "  ";en;",";em$;",";et;",";es
580 close 15

590 print
600 print "all steps done."
610 end
