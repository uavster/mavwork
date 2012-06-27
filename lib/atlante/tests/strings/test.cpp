#include <atlante.h>

#include <stdio.h>
#include <iostream>
using namespace std;

int main(void)
{
	bool error = false;
	try {

		cvgString s = "Local string variable ";
		cout << "Operator + tests:" << endl;
		cout << cvgString("My string plus ") + 1234.34 << endl;
		s += "plus equal another string";
		cout << s << endl;
		cout << cvgString("My string1 plus ") + "string2" << endl;
		cout << "My string1 plus " + cvgString("string2") << endl;
		cout << cvgString("My string1 plus ") + cvgString("string2") << endl;
		cout << cvgString("My string plus ") + 12345 << endl;
		cout << cvgString("My string plus ") + cvgString(12345) << endl;
		cout << cvgString("My string plus ") + CVG_ULONG_MAX + " and another string" << endl;
		cout << 12345 + cvgString(" plus a string") << endl;
		cout << cvgString(12345) + " plus a string " + -6789.34 + " as double" << endl;
		cout << -5.356f + cvgString(" as float plus a string") << endl;
		cout << endl;

		cout << "Operator - tests:" << endl;
		cvgString str = "Remove substring t.e.s.t (without dots): This test is fun";
		str -= "test";
		cout << str << endl;
		cout << cvgString("Re-3mo-31ve-3 so-3me 18446744073709551615n3-3um-3bers") - CVG_LITERAL_ULONG(18446744073709551615) - CVG_LITERAL_INT(-3) << endl;
		cout << cvgString("Remove selected float -8.2.71 and double .3.14. -3.14.-2.2.71") - 2.71f - 3.14159 << endl;
		cout << endl;

		cout << "Operator * tests:" << endl;
		cout << cvgString("Repeat 3 times: ") + cvgString("Word ") * CVG_LITERAL_INT(3) << endl;
		cout << cvgString("Repeat 5.4 times: ") + 5.4f * cvgString("Word ") << endl;
		s *= 2;
		cout << "Double with *= operator: " << s << endl;
		cout << endl;

		cout << "Split tests:" << endl;
		cvgString whole = "part 1, part 2, part 3, part 4, ...";
		cout << "Parts of \"" << whole << "\" split by \", \":" << endl;
		cvgStringVector v = whole.split(", ");
		for (cvgStringVector::iterator it = v.begin(); it < v.end(); it++)
			cout << *it << endl;
		cout << endl;

		cout << "Replace tests:" << endl;
		cout << cvgString("Replace o with 0 from here: The jumping brown fox...").replace("o", "0", 26) << endl;
		cout << cvgString("All lowercase Es are replaced by three").replace("e", 3) << endl;
		cout << cvgString("Mark with X all values similar to pi: 3.142 3.14159 3.2 3.14 3.1").replace(3.14159, 'X') << endl;
		cout << endl;

		cout << "Press ENTER to finish..." << endl;

	} catch(cvgException e) {
		cout << "[Atlante exception] " << e.getMessage() << endl;
		error = true;
	}
	getc(stdin);

	return 0;
}
