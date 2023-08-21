#include <QDir>
#include <QDirIterator>
#include <QSet>
#include <QString>
#include <QTextStream>
#include <QSettings>
#include <time.h>
#include <QDomDocument>
#include <projectproperties.h>
#include <orientation.h>
#include <stack>
#include <dependencygraph.h>
#include <QProcess>
#include <bytebuffer.h>
#include <QCoreApplication>
#include "exportcontext.h"
#include "Utilities.h"
#include "ExportCommon.h"
#include "ExportBuiltin.h"
#include "ExportXml.h"
#include "ExportLua.h"
#include <QCryptographicHash>
#include <QRandomGenerator>

static void addEntryToListIfNotInList(std::vector<QString>& list, const QString& entry)
{
    bool entry_in_list = std::find(list.begin(), list.end(), entry) != list.end();
    if ( !entry_in_list ) {
       list.push_back(entry);
    }
}

static bool readProjectFile(const QString& fileName,
                            ProjectProperties &properties,
                            std::vector<std::pair<QString, QString> > &fileList,
                            std::vector<QString> &folderList,
                            DependencyGraph &dependencyGraph,
							std::set<QString> &noEncryption,
							ExportContext &ctx)
{
    ProjectProperties &properties_ = properties;
    std::vector<std::pair<QString, QString> > &fileList_ = fileList;
    DependencyGraph &dependencyGraph_ = dependencyGraph;

    QDomDocument doc;
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly))
    {
        fprintf(stderr, "Cannot open project file: %s\n", qPrintable(fileName));
        return false;
    }

    if (!doc.setContent(&file))
    {
        fprintf(stderr, "Cannot parse project file: %s\n", qPrintable(fileName));
        file.close();
        return false;
    }

    file.close();

    QDomElement root = doc.documentElement();

    // read properties
    {
        properties_.clear();

        QDomElement properties = root.firstChildElement("properties");
        properties_.loadXml(properties);
    }

    QMap<QString,bool> locked;
    // populate file list, folder list and dependency graph
    {
        fileList_.clear();
        dependencyGraph_.clear();
        noEncryption.clear();
        ctx.noPackage.clear();
        std::vector<std::pair<QString, QString> > dependencies_;

    	//Add lua plugins
        const char* lua_plugins_path = "_LuaPlugins_/";
    	QMap<QString, QString> plugins;
    	QMap<QString, QString> allPlugins=ProjectProperties::availablePlugins();
        bool hasLuaPlugin = false;
        for (QSet<ProjectProperties::Plugin>::const_iterator it=properties_.plugins.begin(); it!=properties_.plugins.end(); it++)
    	{
    		ProjectProperties::Plugin p=*it;
    		if (p.enabled)
    		{
    			QString ppath=allPlugins[p.name];
    			if (!ppath.isEmpty())
    			{
    	    		QFileInfo path(ppath);
    	    		QDir pf=path.dir();
    	    		if (pf.cd("luaplugin"))
    	    		{
                        QDir luaplugin_dir = pf.path();
                        int root_length = luaplugin_dir.path().length();

                        // collect all files in luaplugin and any subdirectory of luaplugin
                        QDirIterator dir_iter(pf.path(), QStringList() << "*", QDir::Files, QDirIterator::Subdirectories);
                        while (dir_iter.hasNext()) {
                            hasLuaPlugin = true;
                            QDir file = dir_iter.next();
                            QString filename = file.path().mid(file.path().lastIndexOf("/") + 1);
                            // 'subtract' luaplugin root path from this path
                            QString rel_path_and_filename = file.path().mid(root_length + 1, file.path().length());
                            QString just_rel_path = rel_path_and_filename.mid(0, rel_path_and_filename.lastIndexOf("/") + 1);

                            addEntryToListIfNotInList(folderList, lua_plugins_path + just_rel_path);
                            fileList_.push_back(std::make_pair(lua_plugins_path + rel_path_and_filename, file.path()));
                            locked[lua_plugins_path + rel_path_and_filename]=true;
                            if (filename.endsWith(".lua"))
                            	dependencyGraph_.addCode(file.path(), true);
                        }
    	    		}
    			}
    		}
    	}
        if ((ctx.deviceFamily == e_Html5)&&properties_.html5_fbinstant) {
			QFileInfo f=QFileInfo("Tools/FBInstant.lua");
            fileList_.push_back(std::make_pair((lua_plugins_path + static_cast<QString>("FBInstant.lua")),
                                               f.absoluteFilePath()));
            locked[lua_plugins_path + static_cast<QString>("FBInstant.lua")]=true;
			dependencyGraph_.addCode(f.absoluteFilePath(),true);
            hasLuaPlugin = true;
 	    }

        if (hasLuaPlugin)
            addEntryToListIfNotInList(folderList, lua_plugins_path);

        std::stack<QDomNode> stack;
        stack.push(doc.documentElement());

        std::vector<QString> dir;

        while (stack.empty() == false)
        {
            QDomNode n = stack.top();
            QDomElement e = n.toElement();
            stack.pop();

            if (n.isNull() == true)
            {
                dir.pop_back();
                continue;
            }

            QString type = e.tagName();

            if (type == "file")
            {
				QString fileName = e.hasAttribute("name")? e.attribute("name"):(e.hasAttribute("source") ? e.attribute("source") : e.attribute("file"));
                QString name = QFileInfo(fileName).fileName();
                bool lock=e.hasAttribute("source");

                QString n;
                for (std::size_t i = 0; i < dir.size(); ++i)
                    n += dir[i] + "/";
                n += name;

                if (locked[n])
                      continue;
                fileList_.push_back(std::make_pair(n, fileName));
                locked[n]=lock;

                if (QFileInfo(fileName).suffix().toLower() == "lua")
                {
                    bool excludeFromExecution = e.hasAttribute("excludeFromExecution") && e.attribute("excludeFromExecution").toInt();
                    dependencyGraph_.addCode(fileName, excludeFromExecution);
                }

                if (e.hasAttribute("excludeFromEncryption") && e.attribute("excludeFromEncryption").toInt())
                	noEncryption.insert(n);
                if (e.hasAttribute("excludeFromPackage") && e.attribute("excludeFromPackage").toInt())
                	ctx.noPackage.insert(n);

                continue;
            }

            if (type == "folder")
            {
                QString name = e.attribute("name");
                dir.push_back(name);

                QString n;
                for (std::size_t i = 0; i < dir.size(); ++i)
                    n += dir[i] + "/";

                folderList.push_back(n);

                stack.push(QDomNode());
            }

            if (type == "dependency")
            {
                QString from = e.attribute("from");
                QString to = e.attribute("to");

                dependencies_.push_back(std::make_pair(from, to));
            }

            QDomNodeList childNodes = n.childNodes();
            for (int i = 0; i < childNodes.size(); ++i)
                stack.push(childNodes.item(i));
        }

        for (size_t i = 0; i < dependencies_.size(); ++i)
            dependencyGraph_.addDependency(dependencies_[i].first,dependencies_[i].second);
    }

    return true;
}

void usage()
{
    fprintf(stderr, "Usage: gdrexport -options <project_file> <output_dir>\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options general: \n");
    fprintf(stderr, "    -platform <platform_name>  #platform to export (ios, android, qtwindows, qtmacosx, qtlinux, winrt, win32, linux, gapp, html5)\n");
    fprintf(stderr, "    -encrypt                   #encrypts code and assets\n");
    fprintf(stderr, "    -encrypt-code              #encrypts code\n");
    fprintf(stderr, "    -encrypt-assets            #encrypts assets\n");
    fprintf(stderr, "    -assets-only               #exports only assets\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options ios/apple: \n");
    fprintf(stderr, "    -bundle <bundle_id>        #bundle id\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options android: \n");
    fprintf(stderr, "    -package <package_name>    #apk package name\n");
    fprintf(stderr, "    -template <template>       #template to use (eclipse, androidstudio)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options qtwindows: \n");
    fprintf(stderr, "    -organization <name>       #organization name\n");
    fprintf(stderr, "    -domain <domain_name>      #domain name\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options qtmacosx: \n");
    fprintf(stderr, "    -organization <name>       #organization name\n");
    fprintf(stderr, "    -domain <domain_name>      #domain name\n");
    fprintf(stderr, "    -bundle <bundle_id>        #bundle id\n");
    fprintf(stderr, "    -signingId <signing_id>    #signing identity\n");
    fprintf(stderr, "    -installerId <signing_id>  #installer signing identity\n");
    fprintf(stderr, "    -category <app category>   #category of your app\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options winrt: \n");
    fprintf(stderr, "    -organization <name>       #organization name\n");
    fprintf(stderr, "    -package <package_name>    #package name\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options html5: \n");
    fprintf(stderr, "    -hostname <name>           #host name the app will be run from\n");
}

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("GiderosMobile");
    QCoreApplication::setOrganizationDomain("giderosmobile.com");
    QCoreApplication::setApplicationName("GiderosStudio");

    QSettings::setDefaultFormat(QSettings::IniFormat);

    QCoreApplication a(argc, argv);

    QStringList arguments = a.arguments();

    ExportContext ctx;
    ctx.deviceFamily=e_None;
    ctx.appicon=NULL;
    ctx.tvicon=NULL;
    ctx.splash_h_image=NULL;
    ctx.splash_v_image=NULL;
    ctx.exportError=false;
    QString projectFileName;
    QString output;
    bool encryptCode = false;
    bool encryptAssets = false;
    bool assetsOnly = false;
    bool player = false;

    //Lookup XML exports scripts
    QMap<QString,QString> xmlExports=ExportXml::availableTargets();

    int i = 1;
    while (i < arguments.size())
    {
        if (arguments[i] == "-platform")
        {
            if (i + 1 >= arguments.size())
            {
                fprintf(stderr, "Missing argument: platform_name\n\n");
                usage();
                return 1;
            }
            QString platform = arguments[i + 1];
            ctx.platform=platform;
            if ((platform.toLower() == "ios")||(platform.toLower() == "apple"))
            {
                ctx.deviceFamily = e_iOS;
            }
            else if (platform.toLower() == "android")
            {
            	ctx.deviceFamily = e_Android;
            }
            else if (platform.toLower() == "qtwindows")
            {
            	ctx.deviceFamily = e_WindowsDesktop;
            }
            else if (platform.toLower() == "qtmacosx")
            {
                ctx.deviceFamily = e_MacOSXDesktop;
            }
            else if (platform.toLower() == "qtlinux")
            {
                ctx.deviceFamily = e_LinuxDesktop;
            }
            else if (platform.toLower() == "winrt")
            {
            	ctx.deviceFamily = e_WinRT;
            }
            else if (platform.toLower() == "win32")
            {
                ctx.deviceFamily = e_Win32;
            }
            else if (platform.toLower() == "linux")
            {
                ctx.deviceFamily = e_Linux;
            }
            else if (platform.toLower() == "gapp")
            {
            	ctx.deviceFamily = e_GApp;
            }
            else if (platform.toLower() == "html5")
            {
            	ctx.deviceFamily = e_Html5;
            }
            else if (xmlExports.contains(platform))
            {
            	ctx.deviceFamily = e_Xml;
            }
            else
            {
                fprintf(stderr, "Unknown argument for platform_name: %s\n\n", qPrintable(platform));
                usage();
                return 1;
            }
            i += 2;
        }
        else if (arguments[i] == "-encrypt")
        {
            encryptCode = true;
            encryptAssets = true;
            i++;
        }
        else if (arguments[i] == "-encrypt-code")
        {
            encryptCode = true;
            i++;
        }
        else if (arguments[i] == "-encrypt-assets")
        {
            encryptAssets = true;
            i++;
        }
        else if (arguments[i] == "-assets-only")
        {
            assetsOnly = true;
            i++;
        }
        else if (arguments[i] == "-player")
        {
            player = true;
            i++;
        }
        else if (arguments[i].startsWith("-"))
        {
            if (i + 1 >= arguments.size())
            {
                fprintf(stderr, "Missing value fo argument: %s\n\n", arguments[i].toStdString().c_str());
                usage();
                return 1;
            }
            ctx.args.insert(arguments[i].remove(0,1), arguments[i + 1]);
            i += 2;
        }
        else
        {
            if (projectFileName.isEmpty())
            {
                projectFileName = arguments[i];
                i++;
            }
            else if (output.isEmpty())
            {
                output = arguments[i];
                i++;
            }
            else
            {
                fprintf(stderr, "Unknown argument: %s\n\n", qPrintable(arguments[i]));
                usage();
                return 1;
            }
        }
    }

    if (projectFileName.isEmpty())
    {
        fprintf(stderr, "Missing argument: project_file\n\n");
        usage();
        return 1;
    }

    if (output.isEmpty())
    {
        fprintf(stderr, "Missing argument: output_dir\n\n");
        usage();
        return 1;
    }

    if (ctx.deviceFamily == e_None)
    {
        fprintf(stderr, "Missing argument: platform_name\n\n");
        usage();
        return 1;
    }

    if (player) //Do not encrypt player
    {
        encryptAssets=false;
        encryptCode=false;
    }
    
    QDir outputDir(output);

    projectFileName = QDir::current().absoluteFilePath(projectFileName);

    QDir cdir = QCoreApplication::applicationDirPath();
    cdir.cdUp();
    QDir::setCurrent(cdir.absolutePath());

    const QString &projectFileName_ = projectFileName;
    ctx.projectFileName_=projectFileName_;
    ctx.base = QFileInfo(projectFileName_).baseName();

    outputDir.mkdir(ctx.base);
    outputDir.cd(ctx.base);
    ctx.outputDir=outputDir;
    ctx.exportDir = QDir(outputDir);

    QByteArray codePrefix("312e68c04c6fd22922b5b232ea6fb3e1");
    QByteArray assetsPrefix("312e68c04c6fd22922b5b232ea6fb3e2");
    QByteArray codePrefixRnd("312e68c04c6fd22922b5b232ea6fb3e1");
    QByteArray assetsPrefixRnd("312e68c04c6fd22922b5b232ea6fb3e2");
    QByteArray encryptionZero(256, '\0');
    QByteArray codeKey(256, '\0');
    QByteArray assetsKey(256, '\0');
    QByteArray randomData(32+32+256+256, '\0');

    {
        QSettings settings;
        if (settings.contains("randomData"))
        {
            randomData = settings.value("randomData").toByteArray();
        }
        else
        {
            for (int i = 0; i < randomData.size(); ++i)
                randomData[i] = QRandomGenerator::global()->generate() % 256;
            settings.setValue("randomData", randomData);
            settings.sync();
        }
    }

    if (encryptCode)
    {
        codeKey = randomData.mid(64,256);
        codePrefixRnd=randomData.mid(0,32);
    }

    if (encryptAssets)
    {
        assetsKey = randomData.mid(64+256,256);
        assetsPrefixRnd=randomData.mid(32,32);
    }

    DependencyGraph dependencyGraph;
    if (readProjectFile(projectFileName, ctx.properties, ctx.fileQueue, ctx.folderList, dependencyGraph, ctx.noEncryption,ctx) == false)
    {
        // error is displayed at readProjectFile function
        return 1;
    }
    std::map<QString,QString> fileMap;
    for (std::size_t j = 0; j < ctx.fileQueue.size(); ++j)
        fileMap[ctx.fileQueue[j].second]=ctx.fileQueue[j].first;
    std::vector<std::pair<QString, bool> > topologicalSort = dependencyGraph.topologicalSort(QFileInfo(projectFileName_).dir(),fileMap);
     for (std::size_t i = 0; i < topologicalSort.size(); ++i)
     {
         int index = -1;
         for (std::size_t j = 0; j < ctx.fileQueue.size(); ++j)
         {
             if (ctx.fileQueue[j].second == topologicalSort[i].first)
             {
                 index = j;
                 break;
             }
         }

         if (index != -1)
         {
             std::pair<QString, QString> item = ctx.fileQueue[index];
             ctx.fileQueue.erase(ctx.fileQueue.begin() + index);
             ctx.fileQueue.push_back(item);
         }
     }

    if (ctx.deviceFamily==e_Html5)
    {
    	if ((!(ctx.args["hostname"].isEmpty()))&&(!ctx.properties.html5_fbinstant))
    	{
        	encryptAssets=true;
        	encryptCode=true;
            codePrefixRnd=randomData.mid(0,32);
            assetsPrefixRnd=randomData.mid(32,32);
            for (int n=0;n<16;n++)
            {
            	QByteArray lrnd=QCryptographicHash::hash((ctx.args["hostname"]+QString::number(n)).toUtf8(),QCryptographicHash::Md5);
            	int lrndn=0;
            	for (int k=n*16;k<(n*16+16);k++)
            	{
            		codeKey[k]=codeKey.at(k)^lrnd.at(lrndn);
            		assetsKey[k]=assetsKey.at(k)^lrnd.at(lrndn);
            		lrndn++;
            	}
            }

    		QByteArray mkey=ctx.args["hostname"].toUtf8();
        	int msize=mkey.size();
        	if (msize>255)
        	{
        		msize=255; //Unlikely to happen
        		mkey.truncate(255);
        	}
        	mkey.append((char)0);
        	msize++;
    		codeKey.replace(0,msize,mkey);
    	}
    	else
    	{
    	    QByteArray zero(1, '\0');
    		codeKey.replace(0,1,zero);
    	}
    }

     ctx.assetsOnly=assetsOnly;
     ctx.player=player;
     ctx.codeKey=codeKey;
     ctx.assetsKey=assetsKey;
     ctx.topologicalSort=topologicalSort;
     ctx.encryptCode=encryptCode;
     ctx.encryptAssets=encryptAssets;
     ctx.appName=ctx.properties.app_name;
     if (ctx.appName.isEmpty())
    	 ctx.appName=ctx.base;

     //Encryption key replacement info
     QStringList wildcards2;
     wildcards2 << "libgideros.so" << "libgiderosvr.so" // Android
                << "libgideros.a" // Apple
                << "gid.dll" //QT Win
                << "libgid.1.dylib" //QT Mac
                << "libgid.so.1.0.0" //QT Linux
                << "libgid.so" //Linux
                << "gideros.WindowsPhone.lib" << "gideros.Windows.lib" //UWP
                << "gideros.html.mem" << "gideros-wasm.wasm"  //HTML
                << "*.exe"; //WIN32
     ctx.wildcards << wildcards2;

     QList<QPair<QByteArray, QByteArray> > replaceList2;
     replaceList2 << qMakePair(QByteArray("9852564f4728e0c11e34ca3eb5fe20b2"), QByteArray("9852564f4728e0cffe34ca3eb5fe20b2"));
     replaceList2 << qMakePair(codePrefix + encryptionZero, codePrefixRnd + codeKey);
     replaceList2 << qMakePair(assetsPrefix + encryptionZero, assetsPrefixRnd + assetsKey);
     ctx.replaceList << replaceList2;

     ctx.noEncryptionExt.insert("ttf"); // TTF files do not pass through gvfs!
     ExportLUA_Init(&ctx);
     if (ctx.deviceFamily==e_Xml)
     {
    	 if (!ExportXml::exportXml(xmlExports[ctx.platform],false,&ctx))
			ctx.exportError=true;
     }
     else
    	 ExportBuiltin::doExport(&ctx);
     ExportLUA_Cleanup(&ctx);

    return ctx.exportError?1:0;
}
