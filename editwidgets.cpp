/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "editwidgets.h"

#include <QDirModel>
#include <QFileDialog>
#include <QFileInfo>
#include <QResizeEvent>
#include <QToolButton>
#include <QLabel>

RuleFileEditor::RuleFileEditor(QWidget* p, const QString& var)
  : QLineEdit(p)
  , Variable(var)
{
  this->ToolButton = new QToolButton(this);
  this->ToolButton->setText("...");
  this->ToolButton->setCursor(QCursor(Qt::ArrowCursor));
  QObject::connect(this->ToolButton, SIGNAL(clicked(bool)), this,
                   SLOT(chooseFile()));
}

RuleFilePathEditor::RuleFilePathEditor(QWidget* p, const QString& var)
  : RuleFileEditor(p, var),
  m_pathEnable(false)
{
  this->setCompleter(new RuleFileCompleter(this, false));
}

RulePathEditor::RulePathEditor(QWidget* p, const QString& var)
  : RuleFileEditor(p, var)
{
  this->setCompleter(new RuleFileCompleter(this, true));
}

void RuleFileEditor::resizeEvent(QResizeEvent* e)
{
  // make the tool button fit on the right side
  int h = e->size().height();
  // move the line edit to make room for the tool button
  this->setContentsMargins(0, 0, h, 0);
  // put the tool button in its place
  this->ToolButton->resize(h, h);
  this->ToolButton->move(this->width() - h, 0);
}
#include <QTreeView>
#include <QDialogButtonBox>
void RuleFilePathEditor::chooseFile()
{
  // choose a file and set it
   //QString path;
  QFileInfo info(this->text());
  QString title;
  if (this->Variable.isEmpty()) {
    title = tr("Select File");
  } else {
    title = tr("Select File for %1");
    title = title.arg(this->Variable);
  }
  emit this->fileDialogExists(true);

  m_dlg = new QFileDialog;

  if (m_pathEnable)
  {
	  m_dlg->setOption(QFileDialog::DontUseNativeDialog, true);
	  m_dlg->setWindowTitle(QString::fromLocal8Bit("选择文件或路径"));
	  QTreeView * pTreeView = m_dlg->findChild<QTreeView *>();
	  if (pTreeView)
	  {
		  connect(pTreeView, &QTreeView::doubleClicked, [=]()
		  {			  
			  m_selectedPath = (m_dlg->selectedFiles().at(0));			  
		  });
	  }
	  QLabel* fileNameLabel = m_dlg->findChild<QLabel*>("fileNameLabel");
	  if (fileNameLabel)
	  {
		  fileNameLabel->setText(QString::fromLocal8Bit("文件或文件夹:"));
	  }
	  QDialogButtonBox *pButton = m_dlg->findChild<QDialogButtonBox *>("buttonBox");
	  
	  disconnect(pButton, SIGNAL(accepted()), m_dlg, SLOT(accept()));//使链接失效
	  connect(pButton, &QDialogButtonBox::accepted,
		  [=](){
		  m_selectedPath = m_dlg->selectedFiles().at(0);
		  m_dlg->close();
	  });//改成自己的槽
	  m_dlg->exec();
  }
  else
  {
	  m_selectedPath =
		  m_dlg->getOpenFileName(this, title, info.absolutePath(), QString(),
		  nullptr, QFileDialog::DontResolveSymlinks);
  }

  emit this->fileDialogExists(false);

  if (!m_selectedPath.isEmpty()) {
	  this->setText(QDir::fromNativeSeparators(m_selectedPath));
  }
  delete m_dlg;
}

void RulePathEditor::chooseFile()
{
  // choose a file and set it
  QString path;
  QString title;
  if (this->Variable.isEmpty()) {
    title = tr("Select Path");
  } else {
    title = tr("Select Path for %1");
    title = title.arg(this->Variable);
  }
  emit this->fileDialogExists(true);
  path = QFileDialog::getExistingDirectory(this, title, this->text(),
                                           QFileDialog::ShowDirsOnly |
                                             QFileDialog::DontResolveSymlinks);
  emit this->fileDialogExists(false);
  if (!path.isEmpty()) {
    this->setText(QDir::fromNativeSeparators(path));
  }
}

// use same QDirModel for all completers
static QDirModel* fileDirModel()
{
  static QDirModel* m = nullptr;
  if (!m) {
    m = new QDirModel();
  }
  return m;
}
static QDirModel* pathDirModel()
{
  static QDirModel* m = nullptr;
  if (!m) {
    m = new QDirModel();
    m->setFilter(QDir::AllDirs | QDir::Drives | QDir::NoDotAndDotDot);
  }
  return m;
}

RuleFileCompleter::RuleFileCompleter(QObject* o, bool dirs)
  : QCompleter(o)
{
  QDirModel* m = dirs ? pathDirModel() : fileDirModel();
  this->setModel(m);
}

QString RuleFileCompleter::pathFromIndex(const QModelIndex& idx) const
{
  return QDir::fromNativeSeparators(QCompleter::pathFromIndex(idx));
}
