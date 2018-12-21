#include <QCommandLineParser>
#include "3dParty/singleapplication.h"
#include "autoCopyWidget.h"

int main(int argc, char *argv[])
{
	SingleApplication a(argc, argv);
	AutoCopyWidget w;
	w.show();
	return a.exec();
}
