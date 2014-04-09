#include <Signals.h>

#include <iostream>

using namespace Signals;
using namespace std;

int main()
{
    Signal<int> notifyInt;
    Connection connection = notifyInt.connect([](int i) { cout << "I got passed " << i << endl; });
    notifyInt.connect([](int i) { cout << "I also got passed a " << i << endl; });
    for (int n = 1; n < 10; n++) notifyInt(n);

    notifyInt.disconnect(connection);
    for (int n = 9; n >= 0; n--) notifyInt(n);

    return 0;
}
