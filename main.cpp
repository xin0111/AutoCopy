#include <QCommandLineParser>
#include "3dParty/singleapplication.h"
#include "autoCopyWidget.h"

#if defined(COPYFILES_STACTIC)
#include <QtCore/QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

int main(int argc, char *argv[])
{
	SingleApplication a(argc, argv);
	AutoCopyWidget w;
	w.show();
	return a.exec();
}
