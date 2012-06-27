#include <stdio.h>
#include <iostream>
#include <string>

using namespace std;

#include <atlante.h>

class myWorker : public cvgThread {
public:
	myWorker() : cvgThread("myWorker") {
		i = 0;
	}

	int i;

	virtual void run() {
//		int a = 0;
//		for (cvg_long b = 0; b < 100000000; b++) { a *= a + b; }
		cout << (i++) << endl;
	}
};

int main(void)
{
	bool error = false;
	try {
		myWorker thread;

		string s = "string test!!";
		cout << s << endl;

		thread.start();

		cvg_long a = CVG_LITERAL_LONG(0x1234567812345678);
		printf("0x%"CVG_PRINTF_PREFIX_LONG"x\n", a);
		
		getc(stdin);

		thread.stop();
		cout << "Thread stopped" << endl;
		getc(stdin);

		thread.start();
		getc(stdin);

	} catch(cvgException e) {
		cout << "[Atlante exception] " << e.getMessage() << endl;
		error = true;
	}
	if (!error) cout << "Thread terminated ok" << endl;
	getc(stdin);

	return 0;
}
