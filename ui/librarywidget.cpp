#include "librarywidget.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <QAction>
#include <QFileIconProvider>

LibraryWidget::LibraryWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	connect(ui.treeWidget, SIGNAL(openRequest(const QString&, const QString&)), this, SIGNAL(openRequest(const QString&, const QString&)));
	connect(ui.treeWidget, SIGNAL(previewRequest(const QString&, const QString&)), this, SIGNAL(previewRequest(const QString&, const QString&)));
	connect(ui.treeWidget, SIGNAL(modificationChanged(bool)), this, SLOT(onModificationChanged(bool)));
	connect(ui.treeWidget, SIGNAL(insertIntoDocument(const QString&)), this, SIGNAL(insertIntoDocument(const QString&)));
	connect(ui.treeWidget, SIGNAL(automaticDownsizingEnabled(const QString&)), this, SIGNAL(automaticDownsizingEnabled(const QString&)));
}

LibraryWidget::~LibraryWidget()
{

}

void LibraryWidget::toXml(QXmlStreamWriter &out) const
{
    ui.treeWidget->toXml(out);
}

void LibraryWidget::toXml(QIODevice &file) const
{
    QXmlStreamWriter out(&file);
    out.setAutoFormatting(true);
    out.writeStartDocument();
    toXml(out);
    out.writeEndDocument();
}

void LibraryWidget::clear()
{
	ui.treeWidget->clear();
}

void LibraryWidget::loadXml(const QString& projectFileName, const QDomDocument& doc)
{
	ui.treeWidget->loadXml(projectFileName, doc);
}

void LibraryWidget::newProject(const QString& projectFileName)
{
	ui.treeWidget->newProject(projectFileName);
}

void LibraryWidget::cloneProject(const QString& projectFileName)
{
    ui.treeWidget->cloneProject(projectFileName);
}

std::vector<std::pair<QString, QString> > LibraryWidget::fileList(bool downsizing, bool webClient, bool justLua) {
    return ui.treeWidget->fileList(downsizing,webClient, justLua);
}

void LibraryWidget::consolidateProject()
{
    ui.treeWidget->consolidateProject();
}

void LibraryWidget::setModified(bool m)
{
	ui.treeWidget->setModified(m);
}
bool LibraryWidget::isModified() const
{
	return ui.treeWidget->isModified();
}

QMap<QString, QString> LibraryWidget::usedPlugins()
{
	return ui.treeWidget->usedPlugins();
}

QString LibraryWidget::fileName(const QString& itemName) const
{
    return ui.treeWidget->fileName(itemName);
}

QString LibraryWidget::itemName(QDir dir, const QString& fileName) const
{
    return ui.treeWidget->itemName(dir, fileName);
}

void LibraryWidget::onModificationChanged(bool m)
{
	//ui.label->setText(tr("Project") + (m ? "*" : ""));
	emit modificationChanged(m);
}
