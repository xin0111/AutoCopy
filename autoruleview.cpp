/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "autoruleview.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMetaProperty>
#include <QSortFilterProxyModel>
#include <QStyle>

#include "editwidgets.h"

// filter for searches
class RuleSearchFilter : public QSortFilterProxyModel
{
public:
  RuleSearchFilter(QObject* o)
    : QSortFilterProxyModel(o)
  {
  }

protected:
  bool filterAcceptsRow(int row, const QModelIndex& p) const override
  {
    QStringList strs;
    const QAbstractItemModel* m = this->sourceModel();
    QModelIndex idx = m->index(row, 0, p);

    // if there are no children, get strings for column 0 and 1
    if (!m->hasChildren(idx)) {
      strs.append(m->data(idx).toString());
      idx = m->index(row, 1, p);
      strs.append(m->data(idx).toString());
    } else {
      // get strings for children entries to compare with
      // instead of comparing with the parent
      int num = m->rowCount(idx);
      for (int i = 0; i < num; i++) {
        QModelIndex tmpidx = m->index(i, 0, idx);
        strs.append(m->data(tmpidx).toString());
        tmpidx = m->index(i, 1, idx);
        strs.append(m->data(tmpidx).toString());
      }
    }

    // check all strings for a match
    foreach (QString const& str, strs) {
      if (str.contains(this->filterRegExp())) {
        return true;
      }
    }

    return false;
  }
};

// filter for searches
class RuleAdvancedFilter : public QSortFilterProxyModel
{
public:
  RuleAdvancedFilter(QObject* o)
    : QSortFilterProxyModel(o)
    , ShowAdvanced(false)
  {
  }

  void setShowAdvanced(bool f)
  {
    this->ShowAdvanced = f;
    this->invalidate();
  }
  bool showAdvanced() const { return this->ShowAdvanced; }

protected:
  bool ShowAdvanced;

  bool filterAcceptsRow(int row, const QModelIndex& p) const override
  {
    const QAbstractItemModel* m = this->sourceModel();
    QModelIndex idx = m->index(row, 0, p);

    // if there are no children
    if (!m->hasChildren(idx)) {
      bool adv = m->data(idx, AutoRuleModel::AdvancedRole).toBool();
      return !adv || this->ShowAdvanced;
    }

    // check children
    int num = m->rowCount(idx);
    for (int i = 0; i < num; i++) {
      bool accept = this->filterAcceptsRow(i, idx);
      if (accept) {
        return true;
      }
    }
    return false;
  }
};

AutoRuleView::AutoRuleView(QWidget* p)
  : QTreeView(p)
{
  // hook up our model and search/filter proxies
  this->CacheModel = new AutoRuleModel(this);
  this->AdvancedFilter = new RuleAdvancedFilter(this);
  this->AdvancedFilter->setSourceModel(this->CacheModel);
  this->AdvancedFilter->setDynamicSortFilter(true);
  this->SearchFilter = new RuleSearchFilter(this);
  this->SearchFilter->setSourceModel(this->AdvancedFilter);
  this->SearchFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  this->SearchFilter->setDynamicSortFilter(true);
  this->setModel(this->SearchFilter);

  // our delegate for creating our editors
  AutoRuleModelDelegate* delegate = new AutoRuleModelDelegate(this);
  this->setItemDelegate(delegate);  

  this->setUniformRowHeights(true);

  this->setEditTriggers(QAbstractItemView::AllEditTriggers);

  // tab, backtab doesn't step through items
  this->setTabKeyNavigation(false);

  this->setRootIsDecorated(false);
}

bool AutoRuleView::event(QEvent* e)
{
  if (e->type() == QEvent::Show) {
    this->header()->setDefaultSectionSize(this->viewport()->width() / 2);
  }
  return QTreeView::event(e);
}

AutoRuleModel* AutoRuleView::cacheModel() const
{
  return this->CacheModel;
}

QModelIndex AutoRuleView::moveCursor(CursorAction act,
                                        Qt::KeyboardModifiers mod)
{
  // want home/end to go to begin/end of rows, not columns
  if (act == MoveHome) {
    return this->model()->index(0, 1);
  }
  if (act == MoveEnd) {
    return this->model()->index(this->model()->rowCount() - 1, 1);
  }
  return QTreeView::moveCursor(act, mod);
}

void AutoRuleView::setShowAdvanced(bool s)
{
#if QT_VERSION >= 040300
  // new 4.3 API that needs to be called.  what about an older Qt?
  this->SearchFilter->invalidate();
#endif

  this->AdvancedFilter->setShowAdvanced(s);
}

bool AutoRuleView::showAdvanced() const
{
  return this->AdvancedFilter->showAdvanced();
}

void AutoRuleView::setSearchFilter(const QString& s)
{
  this->SearchFilter->setFilterFixedString(s);
}

AutoRuleModel::AutoRuleModel(QObject* p)
  : QStandardItemModel(p)
  , EditEnabled(true)
  , NewPropertyCount(0)
  , View(FlatView)
{
  this->ShowNewProperties = true;
  QStringList labels;
  labels << tr("SOURCE") << tr("TARGET");
  this->setHorizontalHeaderLabels(labels);
}

AutoRuleModel::~AutoRuleModel()
{
}

static uint qHash(const AutoCopyProperty& p)
{
  return qHash(p.Key);
}

void AutoRuleModel::setShowNewProperties(bool f)
{
  this->ShowNewProperties = f;
}

void AutoRuleModel::clear()
{
  this->QStandardItemModel::clear();
  this->NewPropertyCount = 0;

  QStringList labels;
  labels << tr("SOURCE") << tr("TARGET");
  this->setHorizontalHeaderLabels(labels);
}

void AutoRuleModel::setProperties(const AutoCopyPropertyList& props)
{
  QSet<AutoCopyProperty> newProps, newProps2;

  if (this->ShowNewProperties) {
    newProps = props.toSet();
    newProps2 = newProps;
    QSet<AutoCopyProperty> oldProps = this->properties().toSet();
    oldProps.intersect(newProps);
    newProps.subtract(oldProps);
    newProps2.subtract(newProps);
  } else {
    newProps2 = props.toSet();
  }

  bool b = this->blockSignals(true);

  this->clear();
  this->NewPropertyCount = newProps.size();

  if (View == FlatView) {
	  AutoCopyPropertyList newP = newProps.toList();
	  AutoCopyPropertyList newP2 = newProps2.toList();
    qSort(newP);
    qSort(newP2);
    int row_count = 0;
    foreach (AutoCopyProperty const& p, newP) {
      this->insertRow(row_count);
      this->setPropertyData(this->index(row_count, 0), p, true);
      row_count++;
    }
    foreach (AutoCopyProperty const& p, newP2) {
      this->insertRow(row_count);
      this->setPropertyData(this->index(row_count, 0), p, false);
      row_count++;
    }
  } else if (this->View == GroupView) {
	  QMap<QString, AutoCopyPropertyList> newPropsTree;
    this->breakProperties(newProps, newPropsTree);
	QMap<QString, AutoCopyPropertyList> newPropsTree2;
    this->breakProperties(newProps2, newPropsTree2);

    QStandardItem* root = this->invisibleRootItem();

	for (QMap<QString, AutoCopyPropertyList>::const_iterator iter =
           newPropsTree.begin();
         iter != newPropsTree.end(); ++iter) {
      QString const& key = iter.key();
	  AutoCopyPropertyList const& props2 = iter.value();

      QList<QStandardItem*> parentItems;
      parentItems.append(
        new QStandardItem(key.isEmpty() ? tr("Ungrouped Entries") : key));
      parentItems.append(new QStandardItem());
      parentItems[0]->setData(QBrush(QColor(255, 100, 100)),
                              Qt::BackgroundColorRole);
      parentItems[1]->setData(QBrush(QColor(255, 100, 100)),
                              Qt::BackgroundColorRole);
      parentItems[0]->setData(1, GroupRole);
      parentItems[1]->setData(1, GroupRole);
      root->appendRow(parentItems);

      int num = props2.size();
      for (int i = 0; i < num; i++) {
        AutoCopyProperty prop = props2[i];
        QList<QStandardItem*> items;
        items.append(new QStandardItem());
        items.append(new QStandardItem());
        parentItems[0]->appendRow(items);
        this->setPropertyData(this->indexFromItem(items[0]), prop, true);
      }
    }

	for (QMap<QString, AutoCopyPropertyList>::const_iterator iter =
           newPropsTree2.begin();
         iter != newPropsTree2.end(); ++iter) {
      QString const& key = iter.key();
	  AutoCopyPropertyList const& props2 = iter.value();

      QStandardItem* parentItem =
        new QStandardItem(key.isEmpty() ? tr("Ungrouped Entries") : key);
      root->appendRow(parentItem);
      parentItem->setData(1, GroupRole);

      int num = props2.size();
      for (int i = 0; i < num; i++) {
        AutoCopyProperty prop = props2[i];
        QList<QStandardItem*> items;
        items.append(new QStandardItem());
        items.append(new QStandardItem());
        parentItem->appendRow(items);
        this->setPropertyData(this->indexFromItem(items[0]), prop, false);
      }
    }
  }

  this->blockSignals(b);
}

AutoRuleModel::ViewType AutoRuleModel::viewType() const
{
  return this->View;
}

void AutoRuleModel::setViewType(AutoRuleModel::ViewType t)
{
  this->View = t;

  AutoCopyPropertyList props = this->properties();
  AutoCopyPropertyList oldProps;
  int numNew = this->NewPropertyCount;
  int numTotal = props.count();
  for (int i = numNew; i < numTotal; i++) {
    oldProps.append(props[i]);
  }

  bool b = this->blockSignals(true);
  this->clear();
  this->setProperties(oldProps);
  this->setProperties(props);
  this->blockSignals(b);
}

void AutoRuleModel::setPropertyData(const QModelIndex& idx1,
                                       const AutoCopyProperty& prop, bool isNew)
{
  QModelIndex idx2 = idx1.sibling(idx1.row(),1);

  this->setData(idx1, prop.Key, Qt::DisplayRole);
  this->setData(idx1, prop.Help, AutoRuleModel::HelpRole);
  this->setData(idx1, prop.KeyType, AutoRuleModel::KeyTypeRole);
  this->setData(idx1, prop.Advanced, AutoRuleModel::AdvancedRole);

  if (prop.ValueType == AutoCopyProperty::BOOL) {
    int check = prop.Value.toBool() ? Qt::Checked : Qt::Unchecked;
    this->setData(idx2, check, Qt::CheckStateRole);
  } else {
    this->setData(idx2, prop.Value, Qt::DisplayRole);
  }
  this->setData(idx2, prop.ValueType, AutoRuleModel::ValueTypeRole);
  this->setData(idx2, prop.Help, AutoRuleModel::HelpRole);

  if (!prop.Strings.isEmpty()) {
    this->setData(idx1, prop.Strings, AutoRuleModel::StringsRole);
  }

  Q_UNUSED(isNew);
  //if (isNew) {
  //  this->setData(idx1, QBrush(QColor(255, 100, 100)),
  //                Qt::BackgroundColorRole);
  //  this->setData(idx2, QBrush(QColor(255, 100, 100)),
  //                Qt::BackgroundColorRole);
  //}
}

void AutoRuleModel::updatePropertyAdvance()
{// 修改之前列表为已拷贝
	int nRow = this->rowCount();
	for (int i = 0; i < nRow;++i)
	{
		this->setData(this->index(i,0), true, AutoRuleModel::AdvancedRole);
	}
}

void AutoRuleModel::getPropertyData(const QModelIndex& idx1,
                                       AutoCopyProperty& prop) const
{
  QModelIndex idx2 = idx1.sibling(idx1.row(), 1);

  prop.Key = this->data(idx1, Qt::DisplayRole).toString();
  prop.Help = this->data(idx1, HelpRole).toString();
  prop.KeyType = static_cast<AutoCopyProperty::PropertyType>(
    this->data(idx1, KeyTypeRole).toInt());
  prop.ValueType = static_cast<AutoCopyProperty::PropertyType>(
	  this->data(idx2, ValueTypeRole).toInt());
  prop.Advanced = this->data(idx1, AdvancedRole).toBool();
  prop.Strings =
    this->data(idx1, AutoRuleModel::StringsRole).toStringList();
  if (prop.ValueType == AutoCopyProperty::BOOL) {
    int check = this->data(idx2, Qt::CheckStateRole).toInt();
	prop.Value = check == Qt::Checked;	
  } else {
    prop.Value = this->data(idx2, Qt::DisplayRole).toString();
  }
}

QString AutoRuleModel::prefix(const QString& s)
{
  QString prefix = s.section('_', 0, 0);
  if (prefix == s) {
    prefix = QString();
  }
  return prefix;
}

void AutoRuleModel::breakProperties(
	const QSet<AutoCopyProperty>& props, QMap<QString, AutoCopyPropertyList>& result)
{
	QMap<QString, AutoCopyPropertyList> tmp;
  // return a map of properties grouped by prefixes, and sorted
  foreach (AutoCopyProperty const& p, props) {
    QString prefix = AutoRuleModel::prefix(p.Key);
    tmp[prefix].append(p);
  }
  // sort it and re-org any properties with only one sub item
  AutoCopyPropertyList reorgProps;
  QMap<QString, AutoCopyPropertyList>::iterator iter;
  for (iter = tmp.begin(); iter != tmp.end();) {
    if (iter->count() == 1) {
      reorgProps.append((*iter)[0]);
      iter = tmp.erase(iter);
    } else {
      qSort(*iter);
      ++iter;
    }
  }
  if (reorgProps.count()) {
    tmp[QString()] += reorgProps;
  }
  result = tmp;
}

AutoCopyPropertyList AutoRuleModel::properties() const
{
	AutoCopyPropertyList props;

  if (!this->rowCount()) {
    return props;
  }

  QVector<QModelIndex> idxs;
  idxs.append(this->index(0, 0));

  // walk the entire model for property entries
  // this works regardless of a flat view or a tree view
  while (!idxs.isEmpty()) {
    QModelIndex idx = idxs.last();
    if (this->hasChildren(idx) && this->rowCount(idx)) {
      idxs.append(this->index(0, 0, idx));
    } else {
      if (!data(idx, GroupRole).toInt()) {
        // get data
        AutoCopyProperty prop;
        this->getPropertyData(idx, prop);
        props.append(prop);
      }

      // go to the next in the tree
      while (!idxs.isEmpty() &&
             (
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) &&                                \
  QT_VERSION < QT_VERSION_CHECK(5, 1, 0)
               (idxs.last().row() + 1) >= rowCount(idxs.last().parent()) ||
#endif
               !idxs.last().sibling(idxs.last().row() + 1, 0).isValid())) {
        idxs.remove(idxs.size() - 1);
      }
      if (!idxs.isEmpty()) {
        idxs.last() = idxs.last().sibling(idxs.last().row() + 1, 0);
      }
    }
  }

  return props;
}

bool AutoRuleModel::insertProperty(AutoCopyProperty::PropertyType keyt,
	AutoCopyProperty::PropertyType valuet, const QString& name,
	const QString& description, const QVariant& value,
	bool advanced)
{
  AutoCopyProperty prop;
  prop.Key = name;
  prop.KeyType = keyt;
  prop.Value = value;
  prop.ValueType = valuet;
  prop.Help = description;
  prop.Advanced = advanced;

  // insert at beginning
  this->insertRow(0);
  this->setPropertyData(this->index(0, 0), prop, true);
  this->NewPropertyCount++;
  return true;
}

void AutoRuleModel::setEditEnabled(bool e)
{
  this->EditEnabled = e;
}

bool AutoRuleModel::editEnabled() const
{
  return this->EditEnabled;
}

int AutoRuleModel::newPropertyCount() const
{
  return this->NewPropertyCount;
}

Qt::ItemFlags AutoRuleModel::flags(const QModelIndex& idx) const
{
  Qt::ItemFlags f = QStandardItemModel::flags(idx);
  if (!this->EditEnabled) {
    f &= ~Qt::ItemIsEditable;
    return f;
  }
  if (AutoCopyProperty::BOOL == this->data(idx, KeyTypeRole + idx.column() ).toInt()) {
    f |= Qt::ItemIsUserCheckable;
  }
  return f;
}

QModelIndex AutoRuleModel::buddy(const QModelIndex& idx) const
{
  if (!this->hasChildren(idx) &&
	  this->data(idx, ValueTypeRole).toInt() != AutoCopyProperty::BOOL) {
    return this->index(idx.row(), 1, idx.parent());
  }
  return idx;
}

AutoRuleModelDelegate::AutoRuleModelDelegate(QObject* p)
  : QItemDelegate(p)
  , FileDialogFlag(false)
{
}

void AutoRuleModelDelegate::setFileDialogFlag(bool f)
{
  this->FileDialogFlag = f;
}

QWidget* AutoRuleModelDelegate::createEditor(
  QWidget* p, const QStyleOptionViewItem& /*option*/,
  const QModelIndex& idx) const
{
  QModelIndex var = idx.sibling(idx.row(), idx.column());
  int type = var.data(AutoRuleModel::KeyTypeRole + idx.column()).toInt();
  if (type == AutoCopyProperty::BOOL) {
    return nullptr;
  }
  if (type == AutoCopyProperty::PATH) {
    RulePathEditor* editor =
      new RulePathEditor(p, var.data(Qt::DisplayRole).toString());
    QObject::connect(editor, SIGNAL(fileDialogExists(bool)), this,
                     SLOT(setFileDialogFlag(bool)));
    return editor;
  }
  if (type == AutoCopyProperty::FILEPATH) {
    RuleFilePathEditor* editor =
      new RuleFilePathEditor(p, var.data(Qt::DisplayRole).toString());
    QObject::connect(editor, SIGNAL(fileDialogExists(bool)), this,
                     SLOT(setFileDialogFlag(bool)));
    return editor;
  }
  if (type == AutoCopyProperty::FILEANDPATH) {
	  RuleFilePathEditor* editor =
		  new RuleFilePathEditor(p, var.data(Qt::DisplayRole).toString());
	  editor->setPathEnable(true);
	  QObject::connect(editor, SIGNAL(fileDialogExists(bool)), this,
		  SLOT(setFileDialogFlag(bool)));
	  return editor;
  }
  if (type == AutoCopyProperty::STRING &&
      var.data(AutoRuleModel::StringsRole).isValid()) {
    RuleComboBox* editor = new RuleComboBox(
      p, var.data(AutoRuleModel::StringsRole).toStringList());
    editor->setFrame(false);
    return editor;
  }

  QLineEdit* editor = new QLineEdit(p);
  editor->setFrame(false);
  return editor;
}

bool AutoRuleModelDelegate::editorEvent(QEvent* e,
                                           QAbstractItemModel* model,
                                           const QStyleOptionViewItem& option,
                                           const QModelIndex& index)
{
  Qt::ItemFlags flags = model->flags(index);
  if (!(flags & Qt::ItemIsUserCheckable) ||
      !(option.state & QStyle::State_Enabled) ||
      !(flags & Qt::ItemIsEnabled)) {
    return false;
  }

  QVariant value = index.data(Qt::CheckStateRole);
  if (!value.isValid()) {
    return false;
  }

  if ((e->type() == QEvent::MouseButtonRelease) ||
      (e->type() == QEvent::MouseButtonDblClick)) {
    // eat the double click events inside the check rect
    if (e->type() == QEvent::MouseButtonDblClick) {
      return true;
    }
  } else if (e->type() == QEvent::KeyPress) {
    if (static_cast<QKeyEvent*>(e)->key() != Qt::Key_Space &&
        static_cast<QKeyEvent*>(e)->key() != Qt::Key_Select) {
      return false;
    }
  } else {
    return false;
  }

  Qt::CheckState state =
    (static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked ? Qt::Unchecked
                                                               : Qt::Checked);
  bool success = model->setData(index, state, Qt::CheckStateRole);
  if (success) {
    this->recordChange(model, index);
  }
  return success;
}

// Issue 205903 fixed in Qt 4.5.0.
// Can remove this function and FileDialogFlag when minimum Qt version is 4.5
bool AutoRuleModelDelegate::eventFilter(QObject* object, QEvent* evt)
{
  // workaround for what looks like a bug in Qt on macOS
  // where it doesn't create a QWidget wrapper for the native file dialog
  // so the Qt library ends up assuming the focus was lost to something else

  if (evt->type() == QEvent::FocusOut && this->FileDialogFlag) {
    return false;
  }
  return QItemDelegate::eventFilter(object, evt);
}

void AutoRuleModelDelegate::setModelData(QWidget* editor,
                                            QAbstractItemModel* model,
                                            const QModelIndex& index) const
{
  QItemDelegate::setModelData(editor, model, index);
  const_cast<AutoRuleModelDelegate*>(this)->recordChange(model, index);
}

QSize AutoRuleModelDelegate::sizeHint(const QStyleOptionViewItem& option,
                                         const QModelIndex& index) const
{
  QSize sz = QItemDelegate::sizeHint(option, index);
  QStyle* style = QApplication::style();

  // increase to checkbox size
  QStyleOptionButton opt;
  opt.QStyleOption::operator=(option);
  sz = sz.expandedTo(
    style->subElementRect(QStyle::SE_ViewItemCheckIndicator, &opt, nullptr)
      .size());

  return sz;
}

QSet<AutoCopyProperty> AutoRuleModelDelegate::changes() const
{
  return mChanges;
}

void AutoRuleModelDelegate::clearChanges()
{
  mChanges.clear();
}

void AutoRuleModelDelegate::recordChange(QAbstractItemModel* model,
                                            const QModelIndex& index)
{
  QModelIndex idx = index;
  QAbstractItemModel* mymodel = model;
  while (qobject_cast<QAbstractProxyModel*>(mymodel)) {
    idx = static_cast<QAbstractProxyModel*>(mymodel)->mapToSource(idx);
    mymodel = static_cast<QAbstractProxyModel*>(mymodel)->sourceModel();
  }
  AutoRuleModel* cache_model = qobject_cast<AutoRuleModel*>(mymodel);
  if (cache_model && idx.isValid()) {
    AutoCopyProperty prop;
    idx = idx.sibling(idx.row(), 0);
    cache_model->getPropertyData(idx, prop);

    // clean out an old one
    QSet<AutoCopyProperty>::iterator iter = mChanges.find(prop);
    if (iter != mChanges.end()) {
      mChanges.erase(iter);
    }
    // now add the new item
    mChanges.insert(prop);
  }
}
