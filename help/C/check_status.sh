#!/bin/sh

# Produce a summary of the statuses of all of the .page topics
# (Not done properly, but it's OK for a rough estimate)
# DO NOT TRANSLATE

echo " "
echo "== NONE =="
grep -l "status=\"none" *.page
echo " "
echo "== STUB =="
grep -l "status=\"stub" *.page
echo " "
echo "== INCOMPLETE =="
grep -l "status=\"incomplete" *.page
echo " "
echo "== DRAFT =="
grep -l "status=\"draft" *.page
echo " "
echo "== REVIEW =="
grep -l "status=\"review" *.page
echo " "
echo "== CANDIDATE =="
grep -l "status=\"candidate" *.page
echo " "
echo "== FINAL =="
grep -l "status=\"final" *.page
echo " "
echo "== OUTDATED =="
grep -l "status=\"outdated" *.page

echo " "
echo " "
echo "== SUMMARY =="
echo "None: 		" `grep "status=\"none" *.page | wc -l`
echo "Stub: 		" `grep "status=\"stub" *.page | wc -l`
echo "Incomplete: 	" `grep "status=\"incomplete" *.page | wc -l`
echo "Draft: 		" `grep "status=\"draft" *.page | wc -l`
echo "Review: 	" `grep "status=\"review" *.page | wc -l`
echo "Candidate: 	" `grep "status=\"candidate" *.page | wc -l`
echo "Final: 		" `grep "status=\"final" *.page | wc -l`
echo "Outdated: 	" `grep "status=\"outdated" *.page | wc -l`
echo " "
