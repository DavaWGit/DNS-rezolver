echo "Test 1: chybový výstup (./dns -s kazi.fit.vutbr.cz)"
program_output1=$(./dns -s kazi.fit.vutbr.cz 2>&1)

output1="Není nastavena žádná adresa k rezoluci."

if [[ "$output1" == "$program_output1" ]]; then
    echo "Výstup je správně."
    echo "Výstup = $program_output1"
else
    echo "Výstup se liší:"
    echo "Očekávaný: $output1"
    echo "Dostáno: $program_output1"
fi

echo ""

echo "Test 2: Klasický A záznam (./dns -s kazi.fit.vutbr.cz www.fit.vut.cz -r)"
program_output2=$(./dns -s kazi.fit.vutbr.cz www.fit.vut.cz -r)

output2="Authoritative: Yes, Recursive: Yes, Truncated: No
Question section (1)
  www.fit.vut.cz., A, IN
Answer section (1)
  www.fit.vut.cz., A, IN, 14400, 147.229.9.26
Authority section (0)
Additional section (0)"

if [[ "$output2" == "$program_output2" ]]; then
    echo "Výstup je správně."
    echo "Výstup = $program_output2"
else
    echo "Výstup se liší:"
    echo "Očekávaný: $output2"
    echo "Dostáno: $program_output2"
fi

echo ""

echo "Test 3: Server IPv4 AAAA záznam (./dns -s 147.229.8.12 www.fit.vut.cz -6)"
echo ""
program_output3=$(./dns -s 147.229.8.12 www.fit.vut.cz -6)
echo "$program_output3"
echo ""

echo "Test 4: Server IPv6 PTR záznam pro IPv6 adresu (./dns -s 147.229.8.12 2001:67c:1220:809::93e5:91a -x)"
echo ""
program_output4=$(./dns -s 147.229.8.12 2001:67c:1220:809::93e5:91a -x)
echo "$program_output4"
echo ""

echo "Test 5: Doménové jméno rekurzivní AAAA záznam (./dns -s kazi.fit.vutbr.cz -r kazi.fit.vutbr.cz -6)"
echo ""
program_output5=$(./dns -s kazi.fit.vutbr.cz -r kazi.fit.vutbr.cz -6)
echo "$program_output5"
echo ""