/******************************************************************************
 *
 * Copyright (C) 1997-2015 by Dimitri van Heesch.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License is hereby
 * granted. No representations are made about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 * See the GNU General Public License for more details.
 *
 * Documents produced by Doxygen are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
 */

#include <locale.h>

#include <qfileinfo.h>
#include <qfile.h>
#include <qdir.h>
#include <qdict.h>
#include <qregexp.h>
#include <qstrlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <qtextcodec.h>
#include <errno.h>
#include <qptrdict.h>
#include <qtextstream.h>

#include <algorithm>
#include <unordered_map>
#include <memory>
#include <cinttypes>

#include "version.h"
#include "doxygen.h"
#include "scanner.h"
#include "entry.h"
#include "index.h"
#include "message.h"
#include "config.h"
#include "util.h"
#include "pre.h"
#include "tagreader.h"
#include "dot.h"
#include "msc.h"
#include "docparser.h"
#include "dirdef.h"
#include "outputlist.h"
#include "declinfo.h"
#include "htmlgen.h"
#include "latexgen.h"
#include "mangen.h"
#include "language.h"
#include "debug.h"
#include "htmlhelp.h"
#include "qhp.h"
#include "ftvhelp.h"
#include "defargs.h"
#include "rtfgen.h"
#include "sqlite3gen.h"
#include "xmlgen.h"
#include "docbookgen.h"
#include "defgen.h"
#include "perlmodgen.h"
#include "reflist.h"
#include "pagedef.h"
#include "bufstr.h"
#include "commentcnv.h"
#include "cmdmapper.h"
#include "searchindex.h"
#include "parserintf.h"
#include "htags.h"
#include "pycode.h"
#include "pyscanner.h"
#include "fortrancode.h"
#include "fortranscanner.h"
#include "xmlcode.h"
#include "sqlcode.h"
#include "code.h"
#include "portable.h"
#include "vhdljjparser.h"
#include "vhdldocgen.h"
#include "vhdlcode.h"
#include "eclipsehelp.h"
#include "cite.h"
#include "markdown.h"
#include "arguments.h"
#include "memberlist.h"
#include "layout.h"
#include "groupdef.h"
#include "classlist.h"
#include "namespacedef.h"
#include "filename.h"
#include "membername.h"
#include "membergroup.h"
#include "docsets.h"
#include "formula.h"
#include "settings.h"
#include "context.h"
#include "fileparser.h"
#include "emoji.h"
#include "plantuml.h"
#include "stlsupport.h"
#include "threadpool.h"
#include "clangparser.h"

// provided by the generated file resources.cpp
extern void initResources();

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <signal.h>
#define HAS_SIGNALS
#endif

// globally accessible variables
ClassSDict      *Doxygen::classSDict = 0;
ClassSDict      *Doxygen::hiddenClasses = 0;
NamespaceSDict  *Doxygen::namespaceSDict = 0;
MemberNameLinkedMap *Doxygen::memberNameLinkedMap = 0;
MemberNameLinkedMap *Doxygen::functionNameLinkedMap = 0;
FileNameLinkedMap *Doxygen::inputNameLinkedMap = 0;
GroupSDict      *Doxygen::groupSDict = 0;
PageSDict       *Doxygen::pageSDict = 0;
PageSDict       *Doxygen::exampleSDict = 0;
StringDict       Doxygen::aliasDict(257);          // aliases
StringSet        Doxygen::inputPaths;
FileNameLinkedMap    *Doxygen::includeNameLinkedMap = 0;     // include names
FileNameLinkedMap    *Doxygen::exampleNameLinkedMap = 0;     // examples
FileNameLinkedMap    *Doxygen::imageNameLinkedMap = 0;       // images
FileNameLinkedMap    *Doxygen::dotFileNameLinkedMap = 0;     // dot files
FileNameLinkedMap    *Doxygen::mscFileNameLinkedMap = 0;     // msc files
FileNameLinkedMap    *Doxygen::diaFileNameLinkedMap = 0;     // dia files
StringUnorderedMap    Doxygen::namespaceAliasMap;      // all namespace aliases
StringDict       Doxygen::tagDestinationDict(257); // all tag locations
StringUnorderedSet Doxygen::expandAsDefinedSet; // all macros that should be expanded
QIntDict<MemberGroupInfo> Doxygen::memGrpInfoDict(1009); // dictionary of the member groups heading
PageDef         *Doxygen::mainPage = 0;
bool             Doxygen::insideMainPage = FALSE; // are we generating docs for the main page?
NamespaceDef    *Doxygen::globalScope = 0;
bool             Doxygen::parseSourcesNeeded = FALSE;
SearchIndexIntf *Doxygen::searchIndex=0;
QDict<DefinitionIntf> *Doxygen::symbolMap = 0;
QDict<Definition> *Doxygen::clangUsrMap = 0;
bool             Doxygen::outputToWizard=FALSE;
QDict<int> *     Doxygen::htmlDirMap = 0;
Cache<std::string,LookupInfo> *Doxygen::lookupCache;
DirSDict        *Doxygen::directories;
SDict<DirRelation> Doxygen::dirRelations(257);
ParserManager   *Doxygen::parserManager = 0;
QCString Doxygen::htmlFileExtension;
bool             Doxygen::suppressDocWarnings = FALSE;
QCString         Doxygen::objDBFileName;
QCString         Doxygen::entryDBFileName;
QCString         Doxygen::filterDBFileName;
IndexList       *Doxygen::indexList;
int              Doxygen::subpageNestingLevel = 0;
bool             Doxygen::userComments = FALSE;
QCString         Doxygen::spaces;
bool             Doxygen::generatingXmlOutput = FALSE;
GenericsSDict   *Doxygen::genericsDict;
DefinesPerFileList Doxygen::macroDefinitions;
bool             Doxygen::clangAssistedParsing = FALSE;

// locally accessible globals
static std::map< std::string, const Entry* > g_classEntries;
static StringVector     g_inputFiles;
static QDict<void>      g_compoundKeywordDict(7);  // keywords recognised as compounds
static OutputList      *g_outputList = 0;          // list of output generating objects
static QDict<FileDef>   g_usingDeclarations(1009); // used classes
static bool             g_successfulRun = FALSE;
static bool             g_dumpSymbolMap = FALSE;
static bool             g_useOutputTemplate = FALSE;

void clearAll()
{
  g_inputFiles.clear();
  //g_excludeNameDict.clear();
  //delete g_outputList; g_outputList=0;

  Doxygen::classSDict->clear();
  Doxygen::namespaceSDict->clear();
  Doxygen::pageSDict->clear();
  Doxygen::exampleSDict->clear();
  Doxygen::inputNameLinkedMap->clear();
  Doxygen::includeNameLinkedMap->clear();
  Doxygen::exampleNameLinkedMap->clear();
  Doxygen::imageNameLinkedMap->clear();
  Doxygen::dotFileNameLinkedMap->clear();
  Doxygen::mscFileNameLinkedMap->clear();
  Doxygen::diaFileNameLinkedMap->clear();
  Doxygen::tagDestinationDict.clear();
  SectionManager::instance().clear();
  CitationManager::instance().clear();
  delete Doxygen::mainPage; Doxygen::mainPage=0;
  FormulaManager::instance().clear();
}

class Statistics
{
  public:
    Statistics() { stats.setAutoDelete(TRUE); }
    void begin(const char *name)
    {
      msg("%s", name);
      stat *entry= new stat(name,0);
      stats.append(entry);
      time.restart();
    }
    void end()
    {
      stats.getLast()->elapsed=((double)time.elapsed())/1000.0;
    }
    void print()
    {
      bool restore=FALSE;
      if (Debug::isFlagSet(Debug::Time))
      {
        Debug::clearFlag("time");
        restore=TRUE;
      }
      msg("----------------------\n");
      QListIterator<stat> sli(stats);
      stat *s;
      for ( sli.toFirst(); (s=sli.current()); ++sli )
      {
        msg("Spent %.3f seconds in %s",s->elapsed,s->name);
      }
      if (restore) Debug::setFlag("time");
    }
  private:
    struct stat
    {
      const char *name;
      double elapsed;
      stat() : name(NULL),elapsed(0) {}
      stat(const char *n, double el) : name(n),elapsed(el) {}
    };
    QList<stat> stats;
    QTime       time;
} g_s;


void statistics()
{
#if 0
  fprintf(stderr,"--- inputNameLinkedMap stats ----\n");
  Doxygen::inputNameLinkedMap->statistics();
  fprintf(stderr,"--- includeNameDict stats ----\n");
  Doxygen::includeNameDict->statistics();
  fprintf(stderr,"--- exampleNameDict stats ----\n");
  Doxygen::exampleNameDict->statistics();
  fprintf(stderr,"--- imageNameDict stats ----\n");
  Doxygen::imageNameDict->statistics();
  fprintf(stderr,"--- dotFileNameDict stats ----\n");
  Doxygen::dotFileNameDict->statistics();
  fprintf(stderr,"--- mscFileNameDict stats ----\n");
  Doxygen::mscFileNameDict->statistics();
  fprintf(stderr,"--- diaFileNameDict stats ----\n");
  Doxygen::diaFileNameDict->statistics();
#endif
  //fprintf(stderr,"--- g_excludeNameDict stats ----\n");
  //g_excludeNameDict.statistics();
  fprintf(stderr,"--- aliasDict stats ----\n");
  Doxygen::aliasDict.statistics();
  fprintf(stderr,"--- tagDestinationDict stats ----\n");
  Doxygen::tagDestinationDict.statistics();
  fprintf(stderr,"--- g_compoundKeywordDict stats ----\n");
  g_compoundKeywordDict.statistics();
  fprintf(stderr,"--- memGrpInfoDict stats ----\n");
  Doxygen::memGrpInfoDict.statistics();
}



static void addMemberDocs(const Entry *root,MemberDef *md, const char *funcDecl,
                   const ArgumentList *al,bool over_load,uint64 spec);
static void findMember(const Entry *root,
                       const QCString &relates,
                       const QCString &type,
                       const QCString &args,
                       QCString funcDecl,
                       bool overloaded,
                       bool isFunc
                      );

enum FindBaseClassRelation_Mode
{
  TemplateInstances,
  DocumentedOnly,
  Undocumented
};

static bool findClassRelation(
                           const Entry *root,
                           Definition *context,
                           ClassDef *cd,
                           const BaseInfo *bi,
                           QDict<int> *templateNames,
                           /*bool insertUndocumented*/
                           FindBaseClassRelation_Mode mode,
                           bool isArtificial
                          );

//----------------------------------------------------------------------------

static Definition *findScopeFromQualifiedName(Definition *startScope,const QCString &n,
                                              FileDef *fileScope,const TagInfo *tagInfo);

static void addPageToContext(PageDef *pd,Entry *root)
{
  if (root->parent()) // add the page to it's scope
  {
    QCString scope = root->parent()->name;
    if (root->parent()->section==Entry::PACKAGEDOC_SEC)
    {
      scope=substitute(scope,".","::");
    }
    scope = stripAnonymousNamespaceScope(scope);
    scope+="::"+pd->name();
    Definition *d = findScopeFromQualifiedName(Doxygen::globalScope,scope,0,root->tagInfo());
    if (d)
    {
      pd->setPageScope(d);
    }
  }
}

static void addRelatedPage(Entry *root)
{
  GroupDef *gd=0;
  for (const Grouping &g : root->groups)
  {
    if (!g.groupname.isEmpty() && (gd=Doxygen::groupSDict->find(g.groupname))) break;
  }
  //printf("---> addRelatedPage() %s gd=%p\n",root->name.data(),gd);
  QCString doc;
  if (root->brief.isEmpty())
  {
    doc=root->doc+root->inbodyDocs;
  }
  else
  {
    doc=root->brief+"\n\n"+root->doc+root->inbodyDocs;
  }

  PageDef *pd = addRelatedPage(root->name,root->args,doc,
      root->docFile,
      root->docLine,
      root->startLine,
      root->sli,
      gd,root->tagInfo(),
      FALSE,
      root->lang
     );
  if (pd)
  {
    pd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
    pd->addSectionsToDefinition(root->anchors);
    pd->setLocalToc(root->localToc);
    addPageToContext(pd,root);
  }
}

static void buildGroupListFiltered(const Entry *root,bool additional, bool includeExternal)
{
  if (root->section==Entry::GROUPDOC_SEC && !root->name.isEmpty() &&
        ((!includeExternal && root->tagInfo()==0) ||
         ( includeExternal && root->tagInfo()!=0))
     )
  {
    if ((root->groupDocType==Entry::GROUPDOC_NORMAL && !additional) ||
        (root->groupDocType!=Entry::GROUPDOC_NORMAL &&  additional))
    {
      GroupDef *gd = Doxygen::groupSDict->find(root->name);
      //printf("Processing group '%s':'%s' add=%d ext=%d gd=%p\n",
      //    root->type.data(),root->name.data(),additional,includeExternal,gd);

      if (gd)
      {
        if ( !gd->hasGroupTitle() )
        {
          gd->setGroupTitle( root->type );
        }
        else if ( root->type.length() > 0 && root->name != root->type && gd->groupTitle() != root->type )
        {
          warn( root->fileName,root->startLine,
              "group %s: ignoring title \"%s\" that does not match old title \"%s\"\n",
              qPrint(root->name), qPrint(root->type), qPrint(gd->groupTitle()) );
        }
        gd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
        gd->setDocumentation( root->doc, root->docFile, root->docLine );
        gd->setInbodyDocumentation( root->inbodyDocs, root->inbodyFile, root->inbodyLine );
        gd->addSectionsToDefinition(root->anchors);
        gd->setRefItems(root->sli);
        gd->setLanguage(root->lang);
      }
      else
      {
        if (root->tagInfo())
        {
          gd = createGroupDef(root->fileName,root->startLine,root->name,root->type,root->tagInfo()->fileName);
          gd->setReference(root->tagInfo()->tagName);
        }
        else
        {
          gd = createGroupDef(root->fileName,root->startLine,root->name,root->type);
        }
        gd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
        // allow empty docs for group
        gd->setDocumentation(!root->doc.isEmpty() ? root->doc : QCString(" "),root->docFile,root->docLine,FALSE);
        gd->setInbodyDocumentation( root->inbodyDocs, root->inbodyFile, root->inbodyLine );
        gd->addSectionsToDefinition(root->anchors);
        Doxygen::groupSDict->append(root->name,gd);
        gd->setRefItems(root->sli);
        gd->setLanguage(root->lang);
      }
    }
  }
  for (const auto &e : root->children()) buildGroupListFiltered(e.get(),additional,includeExternal);
}

static void buildGroupList(const Entry *root)
{
  // --- first process only local groups
  // first process the @defgroups blocks
  buildGroupListFiltered(root,FALSE,FALSE);
  // then process the @addtogroup, @weakgroup blocks
  buildGroupListFiltered(root,TRUE,FALSE);

  // --- then also process external groups
  // first process the @defgroups blocks
  buildGroupListFiltered(root,FALSE,TRUE);
  // then process the @addtogroup, @weakgroup blocks
  buildGroupListFiltered(root,TRUE,TRUE);
}

static void findGroupScope(const Entry *root)
{
  if (root->section==Entry::GROUPDOC_SEC && !root->name.isEmpty() &&
      root->parent() && !root->parent()->name.isEmpty())
  {
    GroupDef *gd;
    if ((gd=Doxygen::groupSDict->find(root->name)))
    {
      QCString scope = root->parent()->name;
      if (root->parent()->section==Entry::PACKAGEDOC_SEC)
      {
        scope=substitute(scope,".","::");
      }
      scope = stripAnonymousNamespaceScope(scope);
      scope+="::"+gd->name();
      Definition *d = findScopeFromQualifiedName(Doxygen::globalScope,scope,0,root->tagInfo());
      if (d)
      {
        gd->setGroupScope(d);
      }
    }
  }
  for (const auto &e : root->children()) findGroupScope(e.get());
}

static void organizeSubGroupsFiltered(const Entry *root,bool additional)
{
  if (root->section==Entry::GROUPDOC_SEC && !root->name.isEmpty())
  {
    if ((root->groupDocType==Entry::GROUPDOC_NORMAL && !additional) ||
        (root->groupDocType!=Entry::GROUPDOC_NORMAL && additional))
    {
      GroupDef *gd;
      if ((gd=Doxygen::groupSDict->find(root->name)))
      {
        //printf("adding %s to group %s\n",root->name.data(),gd->name().data());
        addGroupToGroups(root,gd);
      }
    }
  }
  for (const auto &e : root->children()) organizeSubGroupsFiltered(e.get(),additional);
}

static void organizeSubGroups(const Entry *root)
{
  //printf("Defining groups\n");
  // first process the @defgroups blocks
  organizeSubGroupsFiltered(root,FALSE);
  //printf("Additional groups\n");
  // then process the @addtogroup, @weakgroup blocks
  organizeSubGroupsFiltered(root,TRUE);
}

//----------------------------------------------------------------------

static void buildFileList(const Entry *root)
{
  if (((root->section==Entry::FILEDOC_SEC) ||
        ((root->section & Entry::FILE_MASK) && Config_getBool(EXTRACT_ALL))) &&
      !root->name.isEmpty() && !root->tagInfo() // skip any file coming from tag files
     )
  {
    bool ambig;
    FileDef *fd=findFileDef(Doxygen::inputNameLinkedMap,root->name,ambig);
    if (!fd || ambig)
    {
      int save_ambig = ambig;
      // use the directory of the file to see if the described file is in the same
      // directory as the describing file.
      QCString fn = root->fileName;
      int newIndex=fn.findRev('/');
      fd=findFileDef(Doxygen::inputNameLinkedMap,fn.left(newIndex) + "/" + root->name,ambig);
      if (!fd) ambig = save_ambig;
    }
    //printf("**************** root->name=%s fd=%p\n",root->name.data(),fd);
    if (fd && !ambig)
    {
      //printf("Adding documentation!\n");
      // using FALSE in setDocumentation is small hack to make sure a file
      // is documented even if a \file command is used without further
      // documentation
      fd->setDocumentation(root->doc,root->docFile,root->docLine,FALSE);
      fd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
      fd->addSectionsToDefinition(root->anchors);
      fd->setRefItems(root->sli);
      for (const Grouping &g : root->groups)
      {
        GroupDef *gd=0;
        if (!g.groupname.isEmpty() && (gd=Doxygen::groupSDict->find(g.groupname)))
        {
          gd->addFile(fd);
          fd->makePartOfGroup(gd);
          //printf("File %s: in group %s\n",fd->name().data(),s->data());
        }
      }
    }
    else
    {
      const char *fn = root->fileName.data();
      QCString text(4096);
      text.sprintf("the name '%s' supplied as "
          "the argument in the \\file statement ",
          qPrint(root->name));
      if (ambig) // name is ambiguous
      {
        text+="matches the following input files:\n";
        text+=showFileDefMatches(Doxygen::inputNameLinkedMap,root->name);
        text+="Please use a more specific name by "
          "including a (larger) part of the path!";
      }
      else // name is not an input file
      {
        text+="is not an input file";
      }
      warn(fn,root->startLine,"%s", text.data());
    }
  }
  for (const auto &e : root->children()) buildFileList(e.get());
}

static void addIncludeFile(ClassDef *cd,FileDef *ifd,const Entry *root)
{
  if (
      (!root->doc.stripWhiteSpace().isEmpty() ||
       !root->brief.stripWhiteSpace().isEmpty() ||
       Config_getBool(EXTRACT_ALL)
      ) && root->protection!=Private
     )
  {
    //printf(">>>>>> includeFile=%s\n",root->includeFile.data());

    bool local=Config_getBool(FORCE_LOCAL_INCLUDES);
    QCString includeFile = root->includeFile;
    if (!includeFile.isEmpty() && includeFile.at(0)=='"')
    {
      local = TRUE;
      includeFile=includeFile.mid(1,includeFile.length()-2);
    }
    else if (!includeFile.isEmpty() && includeFile.at(0)=='<')
    {
      local = FALSE;
      includeFile=includeFile.mid(1,includeFile.length()-2);
    }

    bool ambig;
    FileDef *fd=0;
    // see if we need to include a verbatim copy of the header file
    //printf("root->includeFile=%s\n",root->includeFile.data());
    if (!includeFile.isEmpty() &&
        (fd=findFileDef(Doxygen::inputNameLinkedMap,includeFile,ambig))==0
       )
    { // explicit request
      QCString text;
      text.sprintf("the name '%s' supplied as "
                  "the argument of the \\class, \\struct, \\union, or \\include command ",
                  qPrint(includeFile)
                 );
      if (ambig) // name is ambiguous
      {
        text+="matches the following input files:\n";
        text+=showFileDefMatches(Doxygen::inputNameLinkedMap,root->includeFile);
        text+="Please use a more specific name by "
            "including a (larger) part of the path!";
      }
      else // name is not an input file
      {
        text+="is not an input file";
      }
      warn(root->fileName,root->startLine, "%s", text.data());
    }
    else if (includeFile.isEmpty() && ifd &&
        // see if the file extension makes sense
        guessSection(ifd->name())==Entry::HEADER_SEC)
    { // implicit assumption
      fd=ifd;
    }

    // if a file is found, we mark it as a source file.
    if (fd)
    {
      QCString iName = !root->includeName.isEmpty() ?
                       root->includeName : includeFile;
      if (!iName.isEmpty()) // user specified include file
      {
        if (iName.at(0)=='<') local=FALSE; // explicit override
        else if (iName.at(0)=='"') local=TRUE;
        if (iName.at(0)=='"' || iName.at(0)=='<')
        {
          iName=iName.mid(1,iName.length()-2); // strip quotes or brackets
        }
        if (iName.isEmpty())
        {
          iName=fd->name();
        }
      }
      else if (!Config_getList(STRIP_FROM_INC_PATH).empty())
      {
        iName=stripFromIncludePath(fd->absFilePath());
      }
      else // use name of the file containing the class definition
      {
        iName=fd->name();
      }
      if (fd->generateSourceFile()) // generate code for header
      {
        cd->setIncludeFile(fd,iName,local,!root->includeName.isEmpty());
      }
      else // put #include in the class documentation without link
      {
        cd->setIncludeFile(0,iName,local,TRUE);
      }
    }
  }
}

#if 0
static bool addNamespace(Entry *root,ClassDef *cd)
{
  // see if this class is defined inside a namespace
  if (root->section & Entry::COMPOUND_MASK)
  {
    Entry *e = root->parent;
    while (e)
    {
      if (e->section==Entry::NAMESPACE_SEC)
      {
        NamespaceDef *nd=0;
        QCString nsName = stripAnonymousNamespaceScope(e->name);
        //printf("addNameSpace() trying: %s\n",nsName.data());
        if (!nsName.isEmpty() && nsName.at(0)!='@' &&
            (nd=getResolvedNamespace(nsName))
           )
        {
          cd->setNamespace(nd);
          cd->setOuterScope(nd);
          nd->insertClass(cd);
          return TRUE;
        }
      }
      e=e->parent;
    }
  }
  return FALSE;
}
#endif

#if 0
static Definition *findScope(Entry *root,int level=0)
{
  if (root==0) return 0;
  //printf("start findScope name=%s\n",root->name.data());
  Definition *result=0;
  if (root->section&Entry::SCOPE_MASK)
  {
    result = findScope(root->parent,level+1); // traverse to the root of the tree
    if (result)
    {
      //printf("Found %s inside %s at level %d\n",root->name.data(),result->name().data(),level);
      // TODO: look at template arguments
      result = result->findInnerCompound(root->name);
    }
    else // reached the global scope
    {
      // TODO: look at template arguments
      result = Doxygen::globalScope->findInnerCompound(root->name);
      //printf("Found in globalScope %s at level %d\n",result->name().data(),level);
    }
  }
  //printf("end findScope(%s,%d)=%s\n",root->name.data(),
  //       level,result==0 ? "<none>" : result->name().data());
  return result;
}
#endif

/*! returns the Definition object belonging to the first \a level levels of
 *  full qualified name \a name. Creates an artificial scope if the scope is
 *  not found and set the parent/child scope relation if the scope is found.
 */
static Definition *buildScopeFromQualifiedName(const QCString name,
                                               int level,SrcLangExt lang,const TagInfo *tagInfo)
{
  //printf("buildScopeFromQualifiedName(%s) level=%d\n",name.data(),level);
  int i=0;
  int p=0,l;
  Definition *prevScope=Doxygen::globalScope;
  QCString fullScope;
  while (i<level)
  {
    int idx=getScopeFragment(name,p,&l);
    if (idx==-1) return prevScope;
    QCString nsName = name.mid(idx,l);
    if (nsName.isEmpty()) return prevScope;
    if (!fullScope.isEmpty()) fullScope+="::";
    fullScope+=nsName;
    NamespaceDef *nd=Doxygen::namespaceSDict->find(fullScope);
    Definition *innerScope = nd;
    ClassDef *cd=0;
    if (nd==0) cd = getClass(fullScope);
    if (nd==0 && cd) // scope is a class
    {
      innerScope = cd;
    }
    else if (nd==0 && cd==0 && fullScope.find('<')==-1) // scope is not known and could be a namespace!
    {
      // introduce bogus namespace
      //printf("++ adding dummy namespace %s to %s tagInfo=%p\n",nsName.data(),prevScope->name().data(),tagInfo);
      nd=createNamespaceDef(
        "[generated]",1,1,fullScope,
        tagInfo?tagInfo->tagName:QCString(),
        tagInfo?tagInfo->fileName:QCString());
      nd->setLanguage(lang);

      // add namespace to the list
      Doxygen::namespaceSDict->inSort(fullScope,nd);
      innerScope = nd;
    }
    else // scope is a namespace
    {
    }
    if (innerScope)
    {
      // make the parent/child scope relation
      prevScope->addInnerCompound(innerScope);
      innerScope->setOuterScope(prevScope);
    }
    else // current scope is a class, so return only the namespace part...
    {
      return prevScope;
    }
    // proceed to the next scope fragment
    p=idx+l+2;
    prevScope=innerScope;
    i++;
  }
  return prevScope;
}

static Definition *findScopeFromQualifiedName(Definition *startScope,const QCString &n,
                                              FileDef *fileScope,const TagInfo *tagInfo)
{
  //printf("<findScopeFromQualifiedName(%s,%s)\n",startScope ? startScope->name().data() : 0, n.data());
  Definition *resultScope=startScope;
  if (resultScope==0) resultScope=Doxygen::globalScope;
  QCString scope=stripTemplateSpecifiersFromScope(n,FALSE);
  int l1=0,i1;
  i1=getScopeFragment(scope,0,&l1);
  if (i1==-1)
  {
    //printf(">no fragments!\n");
    return resultScope;
  }
  int p=i1+l1,l2=0,i2;
  while ((i2=getScopeFragment(scope,p,&l2))!=-1)
  {
    QCString nestedNameSpecifier = scope.mid(i1,l1);
    Definition *orgScope = resultScope;
    //printf("  nestedNameSpecifier=%s\n",nestedNameSpecifier.data());
    resultScope = resultScope->findInnerCompound(nestedNameSpecifier);
    //printf("  resultScope=%p\n",resultScope);
    if (resultScope==0)
    {
      NamespaceSDict *usedNamespaces;
      if (orgScope==Doxygen::globalScope && fileScope &&
          (usedNamespaces = fileScope->getUsedNamespaces()))
        // also search for used namespaces
      {
        NamespaceSDict::Iterator ni(*usedNamespaces);
        NamespaceDef *nd;
        for (ni.toFirst();((nd=ni.current()) && resultScope==0);++ni)
        {
          // restart search within the used namespace
          resultScope = findScopeFromQualifiedName(nd,n,fileScope,tagInfo);
        }
        if (resultScope)
        {
          // for a nested class A::I in used namespace N, we get
          // N::A::I while looking for A, so we should compare
          // resultScope->name() against scope.left(i2+l2)
          //printf("  -> result=%s scope=%s\n",resultScope->name().data(),scope.data());
          if (rightScopeMatch(resultScope->name(),scope.left(i2+l2)))
          {
            break;
          }
          goto nextFragment;
        }
      }

      // also search for used classes. Complication: we haven't been able
      // to put them in the right scope yet, because we are still resolving
      // the scope relations!
      // Therefore loop through all used classes and see if there is a right
      // scope match between the used class and nestedNameSpecifier.
      QDictIterator<FileDef> ui(g_usingDeclarations);
      FileDef *usedFd;
      for (ui.toFirst();(usedFd=ui.current());++ui)
      {
        //printf("Checking using class %s\n",ui.currentKey());
        if (rightScopeMatch(ui.currentKey(),nestedNameSpecifier))
        {
          // ui.currentKey() is the fully qualified name of nestedNameSpecifier
          // so use this instead.
          QCString fqn = QCString(ui.currentKey())+
                         scope.right(scope.length()-p);
          resultScope = buildScopeFromQualifiedName(fqn,fqn.contains("::"),
                                                    startScope->getLanguage(),0);
          //printf("Creating scope from fqn=%s result %p\n",fqn.data(),resultScope);
          if (resultScope)
          {
            //printf("> Match! resultScope=%s\n",resultScope->name().data());
            return resultScope;
          }
        }
      }

      //printf("> name %s not found in scope %s\n",nestedNameSpecifier.data(),orgScope->name().data());
      return 0;
    }
 nextFragment:
    i1=i2;
    l1=l2;
    p=i2+l2;
  }
  //printf(">findScopeFromQualifiedName scope %s\n",resultScope->name().data());
  return resultScope;
}

std::unique_ptr<ArgumentList> getTemplateArgumentsFromName(
                  const QCString &name,
                  const ArgumentLists &tArgLists)
{
  // for each scope fragment, check if it is a template and advance through
  // the list if so.
  int i,p=0;
  auto alIt = tArgLists.begin();
  while ((i=name.find("::",p))!=-1 && alIt!=tArgLists.end())
  {
    NamespaceDef *nd = Doxygen::namespaceSDict->find(name.left(i));
    if (nd==0)
    {
      ClassDef *cd = getClass(name.left(i));
      if (cd)
      {
        if (!cd->templateArguments().empty())
        {
          ++alIt;
        }
      }
    }
    p=i+2;
  }
  return alIt!=tArgLists.end() ?
         std::make_unique<ArgumentList>(*alIt) :
         std::unique_ptr<ArgumentList>();
}

static
ClassDef::CompoundType convertToCompoundType(int section,uint64 specifier)
{
  ClassDef::CompoundType sec=ClassDef::Class;
  if (specifier&Entry::Struct)
    sec=ClassDef::Struct;
  else if (specifier&Entry::Union)
    sec=ClassDef::Union;
  else if (specifier&Entry::Category)
    sec=ClassDef::Category;
  else if (specifier&Entry::Interface)
    sec=ClassDef::Interface;
  else if (specifier&Entry::Protocol)
    sec=ClassDef::Protocol;
  else if (specifier&Entry::Exception)
    sec=ClassDef::Exception;
  else if (specifier&Entry::Service)
    sec=ClassDef::Service;
  else if (specifier&Entry::Singleton)
    sec=ClassDef::Singleton;

  switch(section)
  {
    //case Entry::UNION_SEC:
    case Entry::UNIONDOC_SEC:
      sec=ClassDef::Union;
      break;
      //case Entry::STRUCT_SEC:
    case Entry::STRUCTDOC_SEC:
      sec=ClassDef::Struct;
      break;
      //case Entry::INTERFACE_SEC:
    case Entry::INTERFACEDOC_SEC:
      sec=ClassDef::Interface;
      break;
      //case Entry::PROTOCOL_SEC:
    case Entry::PROTOCOLDOC_SEC:
      sec=ClassDef::Protocol;
      break;
      //case Entry::CATEGORY_SEC:
    case Entry::CATEGORYDOC_SEC:
      sec=ClassDef::Category;
      break;
      //case Entry::EXCEPTION_SEC:
    case Entry::EXCEPTIONDOC_SEC:
      sec=ClassDef::Exception;
      break;
    case Entry::SERVICEDOC_SEC:
      sec=ClassDef::Service;
      break;
    case Entry::SINGLETONDOC_SEC:
      sec=ClassDef::Singleton;
      break;
  }
  return sec;
}


static void addClassToContext(const Entry *root)
{
  FileDef *fd = root->fileDef();

  QCString scName;
  if (root->parent()->section&Entry::SCOPE_MASK)
  {
     scName=root->parent()->name;
  }
  // name without parent's scope
  QCString fullName = root->name;

  // strip off any template parameters (but not those for specializations)
  fullName=stripTemplateSpecifiersFromScope(fullName);

  // name with scope (if not present already)
  QCString qualifiedName = fullName;
  if (!scName.isEmpty() && !leftScopeMatch(fullName,scName))
  {
    qualifiedName.prepend(scName+"::");
  }

  // see if we already found the class before
  ClassDef *cd = getClass(qualifiedName);

  Debug::print(Debug::Classes,0, "  Found class with name %s (qualifiedName=%s -> cd=%p)\n",
      cd ? qPrint(cd->name()) : qPrint(root->name), qPrint(qualifiedName),cd);

  if (cd)
  {
    fullName=cd->name();
    Debug::print(Debug::Classes,0,"  Existing class %s!\n",qPrint(cd->name()));
    //if (cd->templateArguments()==0)
    //{
    //  //printf("existing ClassDef tempArgList=%p specScope=%s\n",root->tArgList,root->scopeSpec.data());
    //  cd->setTemplateArguments(tArgList);
    //}

    cd->setDocumentation(root->doc,root->docFile,root->docLine);
    cd->setBriefDescription(root->brief,root->briefFile,root->briefLine);

    if ((root->spec&Entry::ForwardDecl)==0 && cd->isForwardDeclared())
    {
      cd->setDefFile(root->fileName,root->startLine,root->startColumn);
      if (root->bodyLine!=-1)
      {
        cd->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
        cd->setBodyDef(fd);
      }
    }

    if (cd->templateArguments().empty() || (cd->isForwardDeclared() && (root->spec&Entry::ForwardDecl)==0))
    {
      // this happens if a template class declared with @class is found
      // before the actual definition or if a forward declaration has different template
      // parameter names.
      std::unique_ptr<ArgumentList> tArgList = getTemplateArgumentsFromName(cd->name(),root->tArgLists);
      if (tArgList)
      {
        cd->setTemplateArguments(*tArgList);
      }
    }

    cd->setCompoundType(convertToCompoundType(root->section,root->spec));

    cd->setMetaData(root->metaData);
  }
  else // new class
  {
    ClassDef::CompoundType sec = convertToCompoundType(root->section,root->spec);

    QCString className;
    QCString namespaceName;
    extractNamespaceName(fullName,className,namespaceName);

    //printf("New class: fullname %s namespace '%s' name='%s' brief='%s' docs='%s'\n",
    //    fullName.data(),namespaceName.data(),className.data(),root->brief.data(),root->doc.data());

    QCString tagName;
    QCString refFileName;
    const TagInfo *tagInfo = root->tagInfo();
    int i;
    if (tagInfo)
    {
      tagName     = tagInfo->tagName;
      refFileName = tagInfo->fileName;
      if (fullName.find("::")!=-1)
        // symbols imported via tag files may come without the parent scope,
        // so we artificially create it here
      {
        buildScopeFromQualifiedName(fullName,fullName.contains("::"),root->lang,tagInfo);
      }
    }
    std::unique_ptr<ArgumentList> tArgList;
    if ((root->lang==SrcLangExt_CSharp || root->lang==SrcLangExt_Java) && (i=fullName.find('<'))!=-1)
    {
      // a Java/C# generic class looks like a C++ specialization, so we need to split the
      // name and template arguments here
      tArgList = stringToArgumentList(root->lang,fullName.mid(i));
      fullName=fullName.left(i);
    }
    else
    {
      tArgList = getTemplateArgumentsFromName(fullName,root->tArgLists);
    }
    cd=createClassDef(tagInfo?tagName:root->fileName,root->startLine,root->startColumn,
        fullName,sec,tagName,refFileName,TRUE,root->spec&Entry::Enum);
    Debug::print(Debug::Classes,0,"  New class '%s' (sec=0x%08x)! #tArgLists=%d tagInfo=%p\n",
        qPrint(fullName),sec,root->tArgLists.size(), tagInfo);
    cd->setDocumentation(root->doc,root->docFile,root->docLine); // copy docs to definition
    cd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
    cd->setLanguage(root->lang);
    cd->setId(root->id);
    cd->setHidden(root->hidden);
    cd->setArtificial(root->artificial);
    cd->setClassSpecifier(root->spec);
    cd->setTypeConstraints(root->typeConstr);
    //printf("new ClassDef %s tempArgList=%p specScope=%s\n",fullName.data(),root->tArgList,root->scopeSpec.data());

    //printf("class %s template args=%s\n",fullName.data(),
    //    tArgList ? tempArgListToString(tArgList,root->lang).data() : "<none>");
    if (tArgList)
    {
      cd->setTemplateArguments(*tArgList);
    }
    cd->setProtection(root->protection);
    cd->setIsStatic(root->stat);

    // file definition containing the class cd
    cd->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
    cd->setBodyDef(fd);

    cd->setMetaData(root->metaData);

    // see if the class is found inside a namespace
    //bool found=addNamespace(root,cd);

    cd->insertUsedFile(fd);

    // add class to the list
    //printf("ClassDict.insert(%s)\n",fullName.data());
    Doxygen::classSDict->append(fullName,cd);

    if (cd->isGeneric()) // generics are also stored in a separate dictionary for fast lookup of instances
    {
      //printf("inserting generic '%s' cd=%p\n",fullName.data(),cd);
      Doxygen::genericsDict->insert(fullName,cd);
    }
  }

  cd->addSectionsToDefinition(root->anchors);
  if (!root->subGrouping) cd->setSubGrouping(FALSE);
  if ((root->spec&Entry::ForwardDecl)==0)
  {
    if (cd->hasDocumentation())
    {
      addIncludeFile(cd,fd,root);
    }
    if (fd && (root->section & Entry::COMPOUND_MASK))
    {
      //printf(">> Inserting class '%s' in file '%s' (root->fileName='%s')\n",
      //    cd->name().data(),
      //    fd->name().data(),
      //    root->fileName.data()
      //   );
      cd->setFileDef(fd);
      fd->insertClass(cd);
    }
  }
  addClassToGroups(root,cd);
  cd->setRefItems(root->sli);
}

//----------------------------------------------------------------------
// build a list of all classes mentioned in the documentation
// and all classes that have a documentation block before their definition.
static void buildClassList(const Entry *root)
{
  if (
        ((root->section & Entry::COMPOUND_MASK) ||
         root->section==Entry::OBJCIMPL_SEC) && !root->name.isEmpty()
     )
  {
    addClassToContext(root);
  }
  for (const auto &e : root->children()) buildClassList(e.get());
}

static void buildClassDocList(const Entry *root)
{
  if (
       (root->section & Entry::COMPOUNDDOC_MASK) && !root->name.isEmpty()
     )
  {
    addClassToContext(root);
  }
  for (const auto &e : root->children()) buildClassDocList(e.get());
}

static void resolveClassNestingRelations()
{
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  for (cli.toFirst();cli.current();++cli) cli.current()->setVisited(FALSE);

  bool done=FALSE;
  int iteration=0;
  while (!done)
  {
    done=TRUE;
    ++iteration;
    ClassDef *cd=0;
    for (cli.toFirst();(cd=cli.current());++cli)
    {
      if (!cd->isVisited())
      {
        QCString name = stripAnonymousNamespaceScope(cd->name());
        //printf("processing=%s, iteration=%d\n",cd->name().data(),iteration);
        // also add class to the correct structural context
        Definition *d = findScopeFromQualifiedName(Doxygen::globalScope,
                                                 name,cd->getFileDef(),0);
        if (d)
        {
          //printf("****** adding %s to scope %s in iteration %d\n",cd->name().data(),d->name().data(),iteration);
          d->addInnerCompound(cd);
          cd->setOuterScope(d);

          // for inline namespace add an alias of the class to the outer scope
          while (d->definitionType()==DefinitionIntf::TypeNamespace)
          {
            NamespaceDef *nd = dynamic_cast<NamespaceDef*>(d);
            //printf("d->isInline()=%d\n",nd->isInline());
            if (nd->isInline())
            {
              d = d->getOuterScope();
              if (d)
              {
                ClassDef *aliasCd = createClassDefAlias(d,cd);
                d->addInnerCompound(aliasCd);
                QCString aliasFullName = d->qualifiedName()+"::"+aliasCd->localName();
                Doxygen::classSDict->append(aliasFullName,aliasCd);
                //printf("adding %s to %s as %s\n",qPrint(aliasCd->name()),qPrint(d->name()),qPrint(aliasFullName));
                aliasCd->setVisited(TRUE);
              }
            }
            else
            {
              break;
            }
          }

          cd->setVisited(TRUE);
          done=FALSE;
        }
        //else
        //{
        //  printf("****** ignoring %s: scope not (yet) found in iteration %d\n",cd->name().data(),iteration);
        //}
      }
    }
  }

  //give warnings for unresolved compounds
  ClassDef *cd=0;
  for (cli.toFirst();(cd=cli.current());++cli)
  {
    if (!cd->isVisited())
    {
      QCString name = stripAnonymousNamespaceScope(cd->name());
      //printf("processing unresolved=%s, iteration=%d\n",cd->name().data(),iteration);
      /// create the scope artificially
      // anyway, so we can at least relate scopes properly.
      Definition *d = buildScopeFromQualifiedName(name,name.contains("::"),cd->getLanguage(),0);
      if (d!=cd && !cd->getDefFileName().isEmpty())
                 // avoid recursion in case of redundant scopes, i.e: namespace N { class N::C {}; }
                 // for this case doxygen assumes the existence of a namespace N::N in which C is to be found!
                 // also avoid warning for stuff imported via a tagfile.
      {
        d->addInnerCompound(cd);
        cd->setOuterScope(d);
        warn(cd->getDefFileName(),cd->getDefLine(),
            "Internal inconsistency: scope for class %s not "
            "found!",name.data()
            );
      }
    }
  }
}

void distributeClassGroupRelations()
{
  //static bool inlineGroupedClasses = Config_getBool(INLINE_GROUPED_CLASSES);
  //if (!inlineGroupedClasses) return;
  //printf("** distributeClassGroupRelations()\n");

  ClassSDict::Iterator cli(*Doxygen::classSDict);
  for (cli.toFirst();cli.current();++cli) cli.current()->setVisited(FALSE);

  ClassDef *cd;
  for (cli.toFirst();(cd=cli.current());++cli)
  {
    //printf("Checking %s\n",cd->name().data());
    // distribute the group to nested classes as well
    if (!cd->isVisited() && cd->partOfGroups()!=0 && cd->getClassSDict())
    {
      //printf("  Candidate for merging\n");
      ClassSDict::Iterator ncli(*cd->getClassSDict());
      ClassDef *ncd;
      GroupDef *gd = cd->partOfGroups()->at(0);
      for (ncli.toFirst();(ncd=ncli.current());++ncli)
      {
        if (ncd->partOfGroups()==0)
        {
          //printf("  Adding %s to group '%s'\n",ncd->name().data(),
          //    gd->groupTitle());
          ncd->makePartOfGroup(gd);
          gd->addClass(ncd);
        }
      }
      cd->setVisited(TRUE); // only visit every class once
    }
  }
}

//----------------------------

static ClassDef *createTagLessInstance(ClassDef *rootCd,ClassDef *templ,const QCString &fieldName)
{
  QCString fullName = removeAnonymousScopes(templ->name());
  if (fullName.right(2)=="::") fullName=fullName.left(fullName.length()-2);
  fullName+="."+fieldName;
  ClassDef *cd = createClassDef(templ->getDefFileName(),
                              templ->getDefLine(),
                              templ->getDefColumn(),
                              fullName,
                              templ->compoundType());
  cd->setDocumentation(templ->documentation(),templ->docFile(),templ->docLine()); // copy docs to definition
  cd->setBriefDescription(templ->briefDescription(),templ->briefFile(),templ->briefLine());
  cd->setLanguage(templ->getLanguage());
  cd->setBodySegment(templ->getDefLine(),templ->getStartBodyLine(),templ->getEndBodyLine());
  cd->setBodyDef(templ->getBodyDef());

  cd->setOuterScope(rootCd->getOuterScope());
  if (rootCd->getOuterScope()!=Doxygen::globalScope)
  {
    rootCd->getOuterScope()->addInnerCompound(cd);
  }

  FileDef *fd = templ->getFileDef();
  if (fd)
  {
    cd->setFileDef(fd);
    fd->insertClass(cd);
  }
  GroupList *groups = rootCd->partOfGroups();
  if ( groups!=0 )
  {
    GroupListIterator gli(*groups);
    GroupDef *gd;
    for (gli.toFirst();(gd=gli.current());++gli)
    {
      cd->makePartOfGroup(gd);
      gd->addClass(cd);
    }
  }
  //printf("** adding class %s based on %s\n",fullName.data(),templ->name().data());
  Doxygen::classSDict->append(fullName,cd);

  MemberList *ml = templ->getMemberList(MemberListType_pubAttribs);
  if (ml)
  {
    MemberListIterator li(*ml);
    MemberDef *md;
    for (li.toFirst();(md=li.current());++li)
    {
      //printf("    Member %s type=%s\n",md->name().data(),md->typeString());
      MemberDef *imd = createMemberDef(md->getDefFileName(),md->getDefLine(),md->getDefColumn(),
                                     md->typeString(),md->name(),md->argsString(),md->excpString(),
                                     md->protection(),md->virtualness(),md->isStatic(),Member,
                                     md->memberType(),
                                     ArgumentList(),ArgumentList(),"");
      imd->setMemberClass(cd);
      imd->setDocumentation(md->documentation(),md->docFile(),md->docLine());
      imd->setBriefDescription(md->briefDescription(),md->briefFile(),md->briefLine());
      imd->setInbodyDocumentation(md->inbodyDocumentation(),md->inbodyFile(),md->inbodyLine());
      imd->setMemberSpecifiers(md->getMemberSpecifiers());
      imd->setMemberGroupId(md->getMemberGroupId());
      imd->setInitializer(md->initializer());
      imd->setMaxInitLines(md->initializerLines());
      imd->setBitfields(md->bitfieldString());
      imd->setLanguage(md->getLanguage());
      cd->insertMember(imd);
    }
  }
  return cd;
}

/** Look through the members of class \a cd and its public members.
 *  If there is a member m of a tag less struct/union,
 *  then we create a duplicate of the struct/union with the name of the
 *  member to identify it.
 *  So if cd has name S, then the tag less struct/union will get name S.m
 *  Since tag less structs can be nested we need to call this function
 *  recursively. Later on we need to patch the member types so we keep
 *  track of the hierarchy of classes we create.
 */
static void processTagLessClasses(ClassDef *rootCd,
                                  ClassDef *cd,
                                  ClassDef *tagParentCd,
                                  const QCString &prefix,int count)
{
  //printf("%d: processTagLessClasses %s\n",count,cd->name().data());
  //printf("checking members for %s\n",cd->name().data());
  if (cd->getClassSDict())
  {
    MemberList *ml = cd->getMemberList(MemberListType_pubAttribs);
    if (ml)
    {
      MemberListIterator li(*ml);
      MemberDef *md;
      for (li.toFirst();(md=li.current());++li)
      {
        QCString type = md->typeString();
        if (type.find("::@")!=-1) // member of tag less struct/union
        {
          ClassSDict::Iterator it(*cd->getClassSDict());
          ClassDef *icd;
          for (it.toFirst();(icd=it.current());++it)
          {
            //printf("  member %s: type='%s'\n",md->name().data(),type.data());
            //printf("  comparing '%s'<->'%s'\n",type.data(),icd->name().data());
            if (type.find(icd->name())!=-1) // matching tag less struct/union
            {
              QCString name = md->name();
              if (md->isAnonymous()) name = "__unnamed__";
              if (!prefix.isEmpty()) name.prepend(prefix+".");
              //printf("    found %s for class %s\n",name.data(),cd->name().data());
              ClassDef *ncd = createTagLessInstance(rootCd,icd,name);
              processTagLessClasses(rootCd,icd,ncd,name,count+1);
              //printf("    addTagged %s to %s\n",ncd->name().data(),tagParentCd->name().data());
              tagParentCd->addTaggedInnerClass(ncd);
              ncd->setTagLessReference(icd);

              // replace tag-less type for generated/original member
              // by newly created class name.
              // note the difference between changing cd and tagParentCd.
              // for the initial call this is the same pointer, but for
              // recursive calls cd is the original tag-less struct (of which
              // there is only one instance) and tagParentCd is the newly
              // generated tagged struct of which there can be multiple instances!
              MemberList *pml = tagParentCd->getMemberList(MemberListType_pubAttribs);
              if (pml)
              {
                MemberListIterator pli(*pml);
                MemberDef *pmd;
                for (pli.toFirst();(pmd=pli.current());++pli)
                {
                  if (pmd->name()==md->name())
                  {
                    pmd->setAccessorType(ncd,substitute(pmd->typeString(),icd->name(),ncd->name()));
                    //pmd->setType(substitute(pmd->typeString(),icd->name(),ncd->name()));
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

static void findTagLessClasses(ClassDef *cd)
{
  if (cd->getClassSDict())
  {
    ClassSDict::Iterator it(*cd->getClassSDict());
    ClassDef *icd;
    for (it.toFirst();(icd=it.current());++it)
    {
      if (icd->name().find("@")==-1) // process all non-anonymous inner classes
      {
        findTagLessClasses(icd);
      }
    }
  }

  processTagLessClasses(cd,cd,cd,"",0); // process tag less inner struct/classes (if any)
}

static void findTagLessClasses()
{
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  ClassDef *cd;
  for (cli.toFirst();(cd=cli.current());++cli) // for each class
  {
    Definition *scope = cd->getOuterScope();
    if (scope && scope->definitionType()!=Definition::TypeClass) // that is not nested
    {
      findTagLessClasses(cd);
    }
  }
}


//----------------------------------------------------------------------
// build a list of all namespaces mentioned in the documentation
// and all namespaces that have a documentation block before their definition.
static void buildNamespaceList(const Entry *root)
{
  if (
       (root->section==Entry::NAMESPACE_SEC ||
        root->section==Entry::NAMESPACEDOC_SEC ||
        root->section==Entry::PACKAGEDOC_SEC
       ) &&
       !root->name.isEmpty()
     )
  {
    //printf("** buildNamespaceList(%s)\n",root->name.data());

    QCString fName = root->name;
    if (root->section==Entry::PACKAGEDOC_SEC)
    {
      fName=substitute(fName,".","::");
    }

    QCString fullName = stripAnonymousNamespaceScope(fName);
    if (!fullName.isEmpty())
    {
      //printf("Found namespace %s in %s at line %d\n",root->name.data(),
      //        root->fileName.data(), root->startLine);
      NamespaceDef *nd;
      if ((nd=Doxygen::namespaceSDict->find(fullName))) // existing namespace
      {
        nd->setDocumentation(root->doc,root->docFile,root->docLine);
        nd->setName(fullName); // change name to match docs
        nd->addSectionsToDefinition(root->anchors);
        nd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
        if (nd->getLanguage()==SrcLangExt_Unknown)
        {
          nd->setLanguage(root->lang);
        }
        if (root->tagInfo()==0) // if we found the namespace in a tag file
                                   // and also in a project file, then remove
                                   // the tag file reference
        {
          nd->setReference("");
          nd->setFileName(fullName);
        }
        nd->setMetaData(root->metaData);

        // file definition containing the namespace nd
        FileDef *fd=root->fileDef();
        // insert the namespace in the file definition
        if (fd) fd->insertNamespace(nd);
        addNamespaceToGroups(root,nd);
        nd->setRefItems(root->sli);
      }
      else // fresh namespace
      {
        QCString tagName;
        QCString tagFileName;
        const TagInfo *tagInfo = root->tagInfo();
        if (tagInfo)
        {
          tagName     = tagInfo->tagName;
          tagFileName = tagInfo->fileName;
        }
        //printf("++ new namespace %s lang=%s tagName=%s\n",fullName.data(),langToString(root->lang).data(),tagName.data());
        nd=createNamespaceDef(tagInfo?tagName:root->fileName,root->startLine,
                             root->startColumn,fullName,tagName,tagFileName,
                             root->type,root->spec&Entry::Published);
        nd->setDocumentation(root->doc,root->docFile,root->docLine); // copy docs to definition
        nd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
        nd->addSectionsToDefinition(root->anchors);
        nd->setHidden(root->hidden);
        nd->setArtificial(root->artificial);
        nd->setLanguage(root->lang);
        nd->setId(root->id);
        nd->setMetaData(root->metaData);
        nd->setInline((root->spec&Entry::Inline)!=0);

        //printf("Adding namespace to group\n");
        addNamespaceToGroups(root,nd);
        nd->setRefItems(root->sli);

        // file definition containing the namespace nd
        FileDef *fd=root->fileDef();
        // insert the namespace in the file definition
        if (fd) fd->insertNamespace(nd);

        // the empty string test is needed for extract all case
        nd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
        nd->insertUsedFile(fd);
        nd->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
        nd->setBodyDef(fd);
        // add class to the list
        Doxygen::namespaceSDict->inSort(fullName,nd);

        // also add namespace to the correct structural context
        Definition *d = findScopeFromQualifiedName(Doxygen::globalScope,fullName,0,tagInfo);
        //printf("adding namespace %s to context %s\n",nd->name().data(),d?d->name().data():"<none>");
        if (d==0) // we didn't find anything, create the scope artificially
                  // anyway, so we can at least relate scopes properly.
        {
          d = buildScopeFromQualifiedName(fullName,fullName.contains("::"),nd->getLanguage(),tagInfo);
          d->addInnerCompound(nd);
          nd->setOuterScope(d);
          // TODO: Due to the order in which the tag file is written
          // a nested class can be found before its parent!
        }
        else
        {
          d->addInnerCompound(nd);
          nd->setOuterScope(d);
          // in case of d is an inline namespace, alias insert nd in the part scope of d.
          while (d->definitionType()==DefinitionIntf::TypeNamespace)
          {
            NamespaceDef *pnd = dynamic_cast<NamespaceDef*>(d);
            if (pnd->isInline())
            {
              d = d->getOuterScope();
              if (d)
              {
                NamespaceDef *aliasNd = createNamespaceDefAlias(d,nd);
                //printf("adding %s to %s\n",qPrint(aliasNd->name()),qPrint(d->name()));
                d->addInnerCompound(aliasNd);
              }
            }
            else
            {
              break;
            }
          }
        }
      }
    }
  }
  for (const auto &e : root->children()) buildNamespaceList(e.get());
}

//----------------------------------------------------------------------

static const NamespaceDef *findUsedNamespace(const NamespaceSDict *unl,
                              const QCString &name)
{
  const NamespaceDef *usingNd =0;
  if (unl)
  {
    //printf("Found namespace dict %d\n",unl->count());
    NamespaceSDict::Iterator unli(*unl);
    const NamespaceDef *und;
    for (unli.toFirst();(und=unli.current());++unli)
    {
      QCString uScope=und->name()+"::";
      usingNd = getResolvedNamespace(uScope+name);
      //printf("Also trying with scope='%s' usingNd=%p\n",(uScope+name).data(),usingNd);
    }
  }
  return usingNd;
}

static void findUsingDirectives(const Entry *root)
{
  if (root->section==Entry::USINGDIR_SEC)
  {
    //printf("Found using directive %s at line %d of %s\n",
    //    root->name.data(),root->startLine,root->fileName.data());
    QCString name=substitute(root->name,".","::");
    if (name.right(2)=="::")
    {
      name=name.left(name.length()-2);
    }
    if (!name.isEmpty())
    {
      const NamespaceDef *usingNd = 0;
      NamespaceDef *nd = 0;
      FileDef      *fd = root->fileDef();
      QCString nsName;

      // see if the using statement was found inside a namespace or inside
      // the global file scope.
      if (root->parent() && root->parent()->section==Entry::NAMESPACE_SEC &&
          (fd==0 || fd->getLanguage()!=SrcLangExt_Java) // not a .java file
         )
      {
        nsName=stripAnonymousNamespaceScope(root->parent()->name);
        if (!nsName.isEmpty())
        {
          nd = getResolvedNamespace(nsName);
        }
      }

      // find the scope in which the 'using' namespace is defined by prepending
      // the possible scopes in which the using statement was found, starting
      // with the most inner scope and going to the most outer scope (i.e.
      // file scope).
      int scopeOffset = nsName.length();
      do
      {
        QCString scope=scopeOffset>0 ?
                      nsName.left(scopeOffset)+"::" : QCString();
        usingNd = getResolvedNamespace(scope+name);
        //printf("Trying with scope='%s' usingNd=%p\n",(scope+name).data(),usingNd);
        if (scopeOffset==0)
        {
          scopeOffset=-1;
        }
        else if ((scopeOffset=nsName.findRev("::",scopeOffset-1))==-1)
        {
          scopeOffset=0;
        }
      } while (scopeOffset>=0 && usingNd==0);

      if (usingNd==0 && nd) // not found, try used namespaces in this scope
                            // or in one of the parent namespace scopes
      {
        const NamespaceDef *pnd = nd;
        while (pnd && usingNd==0)
        {
          // also try with one of the used namespaces found earlier
          usingNd = findUsedNamespace(pnd->getUsedNamespaces(),name);

          // goto the parent
          const Definition *s = pnd->getOuterScope();
          if (s && s->definitionType()==Definition::TypeNamespace)
          {
            pnd = dynamic_cast<const NamespaceDef*>(s);
          }
          else
          {
            pnd = 0;
          }
        }
      }
      if (usingNd==0 && fd) // still nothing, also try used namespace in the
                            // global scope
      {
        usingNd = findUsedNamespace(fd->getUsedNamespaces(),name);
      }

      //printf("%s -> %s\n",name.data(),usingNd?usingNd->name().data():"<none>");

      // add the namespace the correct scope
      if (usingNd)
      {
        //printf("using fd=%p nd=%p\n",fd,nd);
        if (nd)
        {
          //printf("Inside namespace %s\n",nd->name().data());
          nd->addUsingDirective(usingNd);
        }
        else if (fd)
        {
          //printf("Inside file %s\n",fd->name().data());
          fd->addUsingDirective(usingNd);
        }
      }
      else // unknown namespace, but add it anyway.
      {
        //printf("++ new unknown namespace %s lang=%s\n",name.data(),langToString(root->lang).data());
        nd=createNamespaceDef(root->fileName,root->startLine,root->startColumn,name);
        nd->setDocumentation(root->doc,root->docFile,root->docLine); // copy docs to definition
        nd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
        nd->addSectionsToDefinition(root->anchors);
        //printf("** Adding namespace %s hidden=%d\n",name.data(),root->hidden);
        nd->setHidden(root->hidden);
        nd->setArtificial(TRUE);
        nd->setLanguage(root->lang);
        nd->setId(root->id);
        nd->setMetaData(root->metaData);
        nd->setInline((root->spec&Entry::Inline)!=0);

        //QListIterator<Grouping> gli(*root->groups);
        //Grouping *g;
        //for (;(g=gli.current());++gli)
        for (const Grouping &g : root->groups)
        {
          GroupDef *gd=0;
          if (!g.groupname.isEmpty() && (gd=Doxygen::groupSDict->find(g.groupname)))
            gd->addNamespace(nd);
        }

        // insert the namespace in the file definition
        if (fd)
        {
          fd->insertNamespace(nd);
          fd->addUsingDirective(nd);
        }

        // the empty string test is needed for extract all case
        nd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
        nd->insertUsedFile(fd);
        // add class to the list
        Doxygen::namespaceSDict->inSort(name,nd);
        nd->setRefItems(root->sli);
      }
    }
  }
  for (const auto &e : root->children()) findUsingDirectives(e.get());
}

//----------------------------------------------------------------------

static void buildListOfUsingDecls(const Entry *root)
{
  if (root->section==Entry::USINGDECL_SEC &&
      !(root->parent()->section&Entry::COMPOUND_MASK) // not a class/struct member
     )
  {
    QCString name = substitute(root->name,".","::");

    if (g_usingDeclarations.find(name)==0)
    {
      FileDef *fd = root->fileDef();
      if (fd)
      {
        g_usingDeclarations.insert(name,fd);
      }
    }
  }
  for (const auto &e : root->children()) buildListOfUsingDecls(e.get());
}


static void findUsingDeclarations(const Entry *root)
{
  if (root->section==Entry::USINGDECL_SEC &&
      !(root->parent()->section&Entry::COMPOUND_MASK) // not a class/struct member
     )
  {
    //printf("Found using declaration %s at line %d of %s inside section %x\n",
    //   root->name.data(),root->startLine,root->fileName.data(),
    //   rootNav->parent()->section());
    if (!root->name.isEmpty())
    {
      ClassDef *usingCd = 0;
      NamespaceDef *nd = 0;
      FileDef      *fd = root->fileDef();
      QCString scName;

      // see if the using statement was found inside a namespace or inside
      // the global file scope.
      if (root->parent()->section == Entry::NAMESPACE_SEC)
      {
        scName=root->parent()->name;
        if (!scName.isEmpty())
        {
          nd = getResolvedNamespace(scName);
        }
      }

      // Assume the using statement was used to import a class.
      // Find the scope in which the 'using' namespace is defined by prepending
      // the possible scopes in which the using statement was found, starting
      // with the most inner scope and going to the most outer scope (i.e.
      // file scope).

      QCString name = substitute(root->name,".","::"); //Java/C# scope->internal
      usingCd = getClass(name); // try direct lookup first, this is needed to get
                                // builtin STL classes to properly resolve, e.g.
                                // vector -> std::vector
      if (usingCd==0)
      {
        usingCd = const_cast<ClassDef*>(getResolvedClass(nd,fd,name)); // try via resolving (see also bug757509)
      }
      if (usingCd==0)
      {
        usingCd = Doxygen::hiddenClasses->find(name); // check if it is already hidden
      }

      //printf("%s -> %p\n",root->name.data(),usingCd);
      if (usingCd==0) // definition not in the input => add an artificial class
      {
        Debug::print(Debug::Classes,0,"  New using class '%s' (sec=0x%08x)! #tArgLists=%d\n",
             qPrint(name),root->section,root->tArgLists.size());
        usingCd = createClassDef(
                     "<using>",1,1,
                     name,
                     ClassDef::Class);
        Doxygen::hiddenClasses->append(root->name,usingCd);
        usingCd->setArtificial(TRUE);
        usingCd->setLanguage(root->lang);
      }
      else
      {
        Debug::print(Debug::Classes,0,"  Found used class %s in scope=%s\n",
            qPrint(usingCd->name()),
                        nd?qPrint(nd->name()):
                        fd?qPrint(fd->name()):
                        "<unknown>");
      }

      if (nd)
      {
        //printf("Inside namespace %s\n",nd->name().data());
        nd->addUsingDeclaration(usingCd);
      }
      else if (fd)
      {
        //printf("Inside file %s\n",fd->name().data());
        fd->addUsingDeclaration(usingCd);
      }
    }
  }
  for (const auto &e : root->children()) findUsingDeclarations(e.get());
}

//----------------------------------------------------------------------

static void findUsingDeclImports(const Entry *root)
{
  if (root->section==Entry::USINGDECL_SEC &&
      (root->parent()->section&Entry::COMPOUND_MASK) // in a class/struct member
     )
  {
    //printf("Found using declaration %s inside section %x\n",
    //    root->name.data(), root->parent()->section);
    QCString fullName=removeRedundantWhiteSpace(root->parent()->name);
    fullName=stripAnonymousNamespaceScope(fullName);
    fullName=stripTemplateSpecifiersFromScope(fullName);
    ClassDef *cd = getClass(fullName);
    if (cd)
    {
      //printf("found class %s\n",cd->name().data());
      int i=root->name.find("::");
      if (i!=-1)
      {
        QCString scope=root->name.left(i);
        QCString memName=root->name.right(root->name.length()-i-2);
        const ClassDef *bcd = getResolvedClass(cd,0,scope); // todo: file in fileScope parameter
        if (bcd && bcd!=cd)
        {
          //printf("found class %s memName=%s\n",bcd->name().data(),memName.data());
          const MemberNameInfoLinkedMap &mnlm=bcd->memberNameInfoLinkedMap();
          const MemberNameInfo *mni = mnlm.find(memName);
          if (mni)
          {
            for (auto &mi : *mni)
            {
              MemberDef *md = mi->memberDef();
              if (md && md->protection()!=Private)
              {
                //printf("found member %s\n",mni->memberName());
                QCString fileName = root->fileName;
                if (fileName.isEmpty() && root->tagInfo())
                {
                  fileName = root->tagInfo()->tagName;
                }
                const ArgumentList &templAl = md->templateArguments();
                const ArgumentList &al = md->templateArguments();
                std::unique_ptr<MemberDef> newMd { createMemberDef(
                    fileName,root->startLine,root->startColumn,
                    md->typeString(),memName,md->argsString(),
                    md->excpString(),root->protection,root->virt,
                    md->isStatic(),Member,md->memberType(),
                    templAl,al,root->metaData
                    ) };
                newMd->setMemberClass(cd);
                cd->insertMember(newMd.get());
                if (!root->doc.isEmpty() || !root->brief.isEmpty())
                {
                  newMd->setDocumentation(root->doc,root->docFile,root->docLine);
                  newMd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
                  newMd->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
                }
                else
                {
                  newMd->setDocumentation(md->documentation(),md->docFile(),md->docLine());
                  newMd->setBriefDescription(md->briefDescription(),md->briefFile(),md->briefLine());
                  newMd->setInbodyDocumentation(md->inbodyDocumentation(),md->inbodyFile(),md->inbodyLine());
                }
                newMd->setDefinition(md->definition());
                newMd->enableCallGraph(root->callGraph);
                newMd->enableCallerGraph(root->callerGraph);
                newMd->enableReferencedByRelation(root->referencedByRelation);
                newMd->enableReferencesRelation(root->referencesRelation);
                newMd->setBitfields(md->bitfieldString());
                newMd->addSectionsToDefinition(root->anchors);
                newMd->setBodySegment(md->getDefLine(),md->getStartBodyLine(),md->getEndBodyLine());
                newMd->setBodyDef(md->getBodyDef());
                newMd->setInitializer(md->initializer());
                newMd->setMaxInitLines(md->initializerLines());
                newMd->setMemberGroupId(root->mGrpId);
                newMd->setMemberSpecifiers(md->getMemberSpecifiers());
                newMd->setLanguage(root->lang);
                newMd->setId(root->id);
                MemberName *mn = Doxygen::memberNameLinkedMap->add(memName);
                mn->push_back(std::move(newMd));
              }
            }
          }
        }
      }
    }

  }
  for (const auto &e : root->children()) findUsingDeclImports(e.get());
}

//----------------------------------------------------------------------

static void findIncludedUsingDirectives()
{
  // first mark all files as not visited
  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    for (const auto &fd : *fn)
    {
      fd->setVisited(FALSE);
    }
  }
  // then recursively add using directives found in #include files
  // to files that have not been visited.
  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    for (const auto &fd : *fn)
    {
      if (!fd->isVisited())
      {
        //printf("----- adding using directives for file %s\n",fd->name().data());
        fd->addIncludedUsingDirectives();
      }
    }
  }
}

//----------------------------------------------------------------------

static MemberDef *addVariableToClass(
    const Entry *root,
    ClassDef *cd,
    MemberType mtype,
    const QCString &type,
    const QCString &name,
    const QCString &args,
    bool fromAnnScope,
    MemberDef *fromAnnMemb,
    Protection prot,
    Relationship related)
{
  QCString qualScope = cd->qualifiedNameWithTemplateParameters();
  QCString scopeSeparator="::";
  SrcLangExt lang = cd->getLanguage();
  if (lang==SrcLangExt_Java || lang==SrcLangExt_CSharp)
  {
    qualScope = substitute(qualScope,"::",".");
    scopeSeparator=".";
  }
  Debug::print(Debug::Variables,0,
      "  class variable:\n"
      "    '%s' '%s'::'%s' '%s' prot=%d ann=%d init='%s'\n",
      qPrint(type),
      qPrint(qualScope),
      qPrint(name),
      qPrint(args),
      root->protection,
      fromAnnScope,
      qPrint(root->initializer)
              );

  QCString def;
  if (!type.isEmpty())
  {
    if (related || mtype==MemberType_Friend || Config_getBool(HIDE_SCOPE_NAMES))
    {
      if (root->spec&Entry::Alias) // turn 'typedef B A' into 'using A = B'
      {
        def="using "+name+" = "+type.mid(7);
      }
      else
      {
        def=type+" "+name+args;
      }
    }
    else
    {
      if (root->spec&Entry::Alias) // turn 'typedef B C::A' into 'using C::A = B'
      {
        def="using "+qualScope+scopeSeparator+name+" = "+type.mid(7);
      }
      else
      {
        def=type+" "+qualScope+scopeSeparator+name+args;
      }
    }
  }
  else
  {
    if (Config_getBool(HIDE_SCOPE_NAMES))
    {
      def=name+args;
    }
    else
    {
      def=qualScope+scopeSeparator+name+args;
    }
  }
  def.stripPrefix("static ");

  // see if the member is already found in the same scope
  // (this may be the case for a static member that is initialized
  //  outside the class)
  MemberName *mn=Doxygen::memberNameLinkedMap->find(name);
  if (mn)
  {
    for (const auto &md : *mn)
    {
      //printf("md->getClassDef()=%p cd=%p type=[%s] md->typeString()=[%s]\n",
      //    md->getClassDef(),cd,type.data(),md->typeString());
      if (!md->isAlias() &&
          md->getClassDef()==cd &&
          removeRedundantWhiteSpace(type)==md->typeString())
        // member already in the scope
      {

        if (root->lang==SrcLangExt_ObjC &&
            root->mtype==Property &&
            md->memberType()==MemberType_Variable)
        { // Objective-C 2.0 property
          // turn variable into a property
          md->setProtection(root->protection);
          cd->reclassifyMember(md.get(),MemberType_Property);
        }
        addMemberDocs(root,md.get(),def,0,FALSE,root->spec);
        //printf("    Member already found!\n");
        return md.get();
      }
    }
  }

  QCString fileName = root->fileName;
  if (fileName.isEmpty() && root->tagInfo())
  {
    fileName = root->tagInfo()->tagName;
  }

  // new member variable, typedef or enum value
  std::unique_ptr<MemberDef> md { createMemberDef(
      fileName,root->startLine,root->startColumn,
      type,name,args,root->exception,
      prot,Normal,root->stat,related,
      mtype,!root->tArgLists.empty() ? root->tArgLists.back() : ArgumentList(),
      ArgumentList(), root->metaData) };
  md->setTagInfo(root->tagInfo());
  md->setMemberClass(cd); // also sets outer scope (i.e. getOuterScope())
  md->setDocumentation(root->doc,root->docFile,root->docLine);
  md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
  md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
  md->setDefinition(def);
  md->setBitfields(root->bitfields);
  md->addSectionsToDefinition(root->anchors);
  md->setFromAnonymousScope(fromAnnScope);
  md->setFromAnonymousMember(fromAnnMemb);
  //md->setIndentDepth(indentDepth);
  md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
  md->setInitializer(root->initializer);
  md->setMaxInitLines(root->initLines);
  md->setMemberGroupId(root->mGrpId);
  md->setMemberSpecifiers(root->spec);
  md->setReadAccessor(root->read);
  md->setWriteAccessor(root->write);
  md->enableCallGraph(root->callGraph);
  md->enableCallerGraph(root->callerGraph);
  md->enableReferencedByRelation(root->referencedByRelation);
  md->enableReferencesRelation(root->referencesRelation);
  md->setHidden(root->hidden);
  md->setArtificial(root->artificial);
  md->setLanguage(root->lang);
  md->setId(root->id);
  addMemberToGroups(root,md.get());
  md->setBodyDef(root->fileDef());

  //printf("    New member adding to %s (%p)!\n",cd->name().data(),cd);
  cd->insertMember(md.get());
  md->setRefItems(root->sli);

  //TODO: insert FileDef instead of filename strings.
  cd->insertUsedFile(root->fileDef());
  root->markAsProcessed();

  //printf("    Adding member=%s\n",md->name().data());
  // add the member to the global list
  MemberDef *result = md.get();
  mn = Doxygen::memberNameLinkedMap->add(name);
  mn->push_back(std::move(md));

  return result;
}

//----------------------------------------------------------------------

static MemberDef *addVariableToFile(
    const Entry *root,
    MemberType mtype,
    const QCString &scope,
    const QCString &type,
    const QCString &name,
    const QCString &args,
    bool fromAnnScope,
    /*int indentDepth,*/
    MemberDef *fromAnnMemb)
{
  Debug::print(Debug::Variables,0,
      "  global variable:\n"
      "    file='%s' type='%s' scope='%s' name='%s' args='%s' prot=`%d mtype=%d lang=%d\n",
      qPrint(root->fileName),
      qPrint(type),
      qPrint(scope),
      qPrint(name),
      qPrint(args),
      root->protection,
      mtype,
      root->lang
              );

  FileDef *fd = root->fileDef();

  // see if we have a typedef that should hide a struct or union
  if (mtype==MemberType_Typedef && Config_getBool(TYPEDEF_HIDES_STRUCT))
  {
    QCString ttype = type;
    ttype.stripPrefix("typedef ");
    if (ttype.left(7)=="struct " || ttype.left(6)=="union ")
    {
      ttype.stripPrefix("struct ");
      ttype.stripPrefix("union ");
      static QRegExp re("[a-z_A-Z][a-z_A-Z0-9]*");
      int l,s;
      s = re.match(ttype,0,&l);
      if (s>=0)
      {
        QCString typeValue = ttype.mid(s,l);
        ClassDef *cd = getClass(typeValue);
        if (cd)
        {
          // this typedef should hide compound name cd, so we
          // change the name that is displayed from cd.
          cd->setClassName(name);
          cd->setDocumentation(root->doc,root->docFile,root->docLine);
          cd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
          return 0;
        }
      }
    }
  }

  // see if the function is inside a namespace
  NamespaceDef *nd = 0;
  if (!scope.isEmpty())
  {
    if (scope.find('@')!=-1) return 0; // anonymous scope!
    //nscope=removeAnonymousScopes(scope);
    //if (!nscope.isEmpty())
    //{
    nd = getResolvedNamespace(scope);
    //}
  }
  QCString def;

  // determine the definition of the global variable
  if (nd && !nd->isAnonymous() &&
      !Config_getBool(HIDE_SCOPE_NAMES)
     )
    // variable is inside a namespace, so put the scope before the name
  {
    SrcLangExt lang = nd->getLanguage();
    QCString sep=getLanguageSpecificSeparator(lang);

    if (!type.isEmpty())
    {
      if (root->spec&Entry::Alias) // turn 'typedef B NS::A' into 'using NS::A = B'
      {
        def="using "+nd->name()+sep+name+" = "+type;
      }
      else // normal member
      {
        def=type+" "+nd->name()+sep+name+args;
      }
    }
    else
    {
      def=nd->name()+sep+name+args;
    }
  }
  else
  {
    if (!type.isEmpty() && !root->name.isEmpty())
    {
      if (name.at(0)=='@') // dummy variable representing anonymous union
      {
        def=type;
      }
      else
      {
        if (root->spec&Entry::Alias) // turn 'typedef B A' into 'using A = B'
        {
          def="using "+root->name+" = "+type.mid(7);
        }
        else // normal member
        {
          def=type+" "+name+args;
        }
      }
    }
    else
    {
      def=name+args;
    }
  }
  def.stripPrefix("static ");

  MemberName *mn=Doxygen::functionNameLinkedMap->find(name);
  if (mn)
  {
    //QCString nscope=removeAnonymousScopes(scope);
    //NamespaceDef *nd=0;
    //if (!nscope.isEmpty())
    if (!scope.isEmpty())
    {
      nd = getResolvedNamespace(scope);
    }
    for (const auto &md : *mn)
    {
      if (!md->isAlias() &&
          ((nd==0 && md->getNamespaceDef()==0 && md->getFileDef() &&
            root->fileName==md->getFileDef()->absFilePath()
           ) // both variable names in the same file
           || (nd!=0 && md->getNamespaceDef()==nd) // both in same namespace
          )
          && !md->isDefine() // function style #define's can be "overloaded" by typedefs or variables
          && !md->isEnumerate() // in C# an enum value and enum can have the same name
         )
        // variable already in the scope
      {
        bool isPHPArray = md->getLanguage()==SrcLangExt_PHP &&
                          md->argsString()!=args &&
                          args.find('[')!=-1;
        bool staticsInDifferentFiles =
                          root->stat && md->isStatic() &&
                          root->fileName!=md->getDefFileName();

        if (md->getFileDef() &&
            !isPHPArray && // not a php array
            !staticsInDifferentFiles
           )
          // not a php array variable
        {
          Debug::print(Debug::Variables,0,
              "    variable already found: scope=%s\n",qPrint(md->getOuterScope()->name()));
          addMemberDocs(root,md.get(),def,0,FALSE,root->spec);
          md->setRefItems(root->sli);
          // if md is a variable forward declaration and root is the definition that
          // turn md into the definition
          if (!root->explicitExternal && md->isExternal())
          {
            md->setDeclFile(md->getDefFileName(),md->getDefLine(),md->getDefColumn());
            md->setExplicitExternal(FALSE,root->fileName,root->startLine,root->startColumn);
          }
          // if md is the definition and root point at a declaration, then add the
          // declaration info
          else if (root->explicitExternal && !md->isExternal())
          {
            md->setDeclFile(root->fileName,root->startLine,root->startColumn);
          }
          return md.get();
        }
      }
    }
  }

  QCString fileName = root->fileName;
  if (fileName.isEmpty() && root->tagInfo())
  {
    fileName = root->tagInfo()->tagName;
  }

  Debug::print(Debug::Variables,0,
    "    new variable, nd=%s tagInfo=%p!\n",nd?qPrint(nd->name()):"<global>",root->tagInfo());
  // new global variable, enum value or typedef
  std::unique_ptr<MemberDef> md { createMemberDef(
      fileName,root->startLine,root->startColumn,
      type,name,args,0,
      root->protection, Normal,root->stat,Member,
      mtype,!root->tArgLists.empty() ? root->tArgLists.back() : ArgumentList(),
      ArgumentList(), root->metaData) };
  md->setTagInfo(root->tagInfo());
  md->setMemberSpecifiers(root->spec);
  md->setDocumentation(root->doc,root->docFile,root->docLine);
  md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
  md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
  md->addSectionsToDefinition(root->anchors);
  md->setFromAnonymousScope(fromAnnScope);
  md->setFromAnonymousMember(fromAnnMemb);
  md->setInitializer(root->initializer);
  md->setMaxInitLines(root->initLines);
  md->setMemberGroupId(root->mGrpId);
  md->setDefinition(def);
  md->setLanguage(root->lang);
  md->setId(root->id);
  md->enableCallGraph(root->callGraph);
  md->enableCallerGraph(root->callerGraph);
  md->enableReferencedByRelation(root->referencedByRelation);
  md->enableReferencesRelation(root->referencesRelation);
  md->setExplicitExternal(root->explicitExternal,fileName,root->startLine,root->startColumn);
  //md->setOuterScope(fd);
  if (!root->explicitExternal)
  {
    md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
    md->setBodyDef(fd);
  }
  addMemberToGroups(root,md.get());

  md->setRefItems(root->sli);
  if (nd && !nd->isAnonymous())
  {
    md->setNamespace(nd);
    nd->insertMember(md.get());
  }

  // add member to the file (we do this even if we have already inserted
  // it into the namespace.
  if (fd)
  {
    md->setFileDef(fd);
    fd->insertMember(md.get());
  }

  root->markAsProcessed();

  // add member definition to the list of globals
  MemberDef *result = md.get();
  mn = Doxygen::functionNameLinkedMap->add(name);
  mn->push_back(std::move(md));

  return result;
}

/*! See if the return type string \a type is that of a function pointer
 *  \returns -1 if this is not a function pointer variable or
 *           the index at which the closing brace of (...*name) was found.
 */
static int findFunctionPtr(const QCString &type,int lang, int *pLength=0)
{
  if (lang == SrcLangExt_Fortran || lang == SrcLangExt_VHDL)
  {
    return -1; // Fortran and VHDL do not have function pointers
  }
  static const QRegExp re("([^)]*[\\*\\^][^)]*)");
  int i=-1,l;
  int bb=type.find('<');
  int be=type.findRev('>');
  if (!type.isEmpty() &&             // return type is non-empty
      (i=re.match(type,0,&l))!=-1 && // contains (...*...)
      type.find("operator")==-1 &&   // not an operator
      (type.find(")(")==-1 || type.find("typedef ")!=-1) &&
                                    // not a function pointer return type
      !(bb<i && i<be) // bug665855: avoid treating "typedef A<void (T*)> type" as a function pointer
     )
  {
    if (pLength) *pLength=l;
    //printf("findFunctionPtr=%d\n",i);
    return i;
  }
  else
  {
    //printf("findFunctionPtr=%d\n",-1);
    return -1;
  }
}


/*! Returns TRUE iff \a type is a class within scope \a context.
 *  Used to detect variable declarations that look like function prototypes.
 */
static bool isVarWithConstructor(const Entry *root)
{
  static QRegExp initChars("[0-9\"'&*!^]+");
  static QRegExp idChars("[a-z_A-Z][a-z_A-Z0-9]*");
  bool result=FALSE;
  bool typeIsClass;
  QCString type;
  Definition *ctx = 0;
  FileDef *fd = 0;
  int ti;

  //printf("isVarWithConstructor(%s)\n",rootNav->name().data());
  if (root->parent()->section & Entry::COMPOUND_MASK)
  { // inside a class
    result=FALSE;
    goto done;
  }
  else if ((fd = root->fileDef()) &&
            (fd->name().right(2)==".c" || fd->name().right(2)==".h")
          )
  { // inside a .c file
    result=FALSE;
    goto done;
  }
  if (root->type.isEmpty())
  {
    result=FALSE;
    goto done;
  }
  if (!root->parent()->name.isEmpty())
  {
    ctx=Doxygen::namespaceSDict->find(root->parent()->name);
  }
  type = root->type;
  // remove qualifiers
  findAndRemoveWord(type,"const");
  findAndRemoveWord(type,"static");
  findAndRemoveWord(type,"volatile");
  //if (type.left(6)=="const ") type=type.right(type.length()-6);
  typeIsClass=getResolvedClass(ctx,fd,type)!=0;
  if (!typeIsClass && (ti=type.find('<'))!=-1)
  {
    typeIsClass=getResolvedClass(ctx,fd,type.left(ti))!=0;
  }
  if (typeIsClass) // now we still have to check if the arguments are
                   // types or values. Since we do not have complete type info
                   // we need to rely on heuristics :-(
  {
    //printf("typeIsClass\n");
    if (root->argList.empty())
    {
      result=FALSE; // empty arg list -> function prototype.
      goto done;
    }
    for (const Argument &a : root->argList)
    {
      if (!a.name.isEmpty() || !a.defval.isEmpty())
      {
        if (a.name.find(initChars)==0)
        {
          result=TRUE;
        }
        else
        {
          result=FALSE; // arg has (type,name) pair -> function prototype
        }
        goto done;
      }
      if (a.type.isEmpty() || getResolvedClass(ctx,fd,a.type)!=0)
      {
        result=FALSE; // arg type is a known type
        goto done;
      }
      if (checkIfTypedef(ctx,fd,a.type))
      {
         //printf("%s:%d: false (arg is typedef)\n",__FILE__,__LINE__);
         result=FALSE; // argument is a typedef
         goto done;
      }
      if (a.type.at(a.type.length()-1)=='*' ||
          a.type.at(a.type.length()-1)=='&')
                     // type ends with * or & => pointer or reference
      {
        result=FALSE;
        goto done;
      }
      if (a.type.find(initChars)==0)
      {
        result=TRUE; // argument type starts with typical initializer char
        goto done;
      }
      QCString resType=resolveTypeDef(ctx,a.type);
      if (resType.isEmpty()) resType=a.type;
      int len;
      if (idChars.match(resType,0,&len)==0) // resType starts with identifier
      {
        resType=resType.left(len);
        //printf("resType=%s\n",resType.data());
        if (resType=="int"    || resType=="long" || resType=="float" ||
            resType=="double" || resType=="char" || resType=="signed" ||
            resType=="const"  || resType=="unsigned" || resType=="void")
        {
          result=FALSE; // type keyword -> function prototype
          goto done;
        }
      }
    }
    result=TRUE;
  }

done:
  //printf("isVarWithConstructor(%s,%s)=%d\n",rootNav->parent()->name().data(),
  //                                          root->type.data(),result);
  return result;
}

static void addVariable(const Entry *root,int isFuncPtr=-1)
{
    static bool sliceOpt = Config_getBool(OPTIMIZE_OUTPUT_SLICE);

    Debug::print(Debug::Variables,0,
                  "VARIABLE_SEC: \n"
                  "  type='%s' name='%s' args='%s' bodyLine=%d mGrpId=%d relates='%s'\n",
                   qPrint(root->type),
                   qPrint(root->name),
                   qPrint(root->args),
                   root->bodyLine,
                   root->mGrpId,
                   qPrint(root->relates)
                );
    //printf("root->parent->name=%s\n",root->parent->name.data());

    QCString type = root->type;
    QCString name = root->name;
    QCString args = root->args;
    if (type.isEmpty() && name.find("operator")==-1 &&
        (name.find('*')!=-1 || name.find('&')!=-1))
    {
      // recover from parse error caused by redundant braces
      // like in "int *(var[10]);", which is parsed as
      // type="" name="int *" args="(var[10])"

      type=name;
      static const QRegExp reName("[a-z_A-Z][a-z_A-Z0-9]*");
      int l=0;
      int j=0;
      int i=args.isEmpty() ? -1 : reName.match(args,0,&l);
      if (i!=-1)
      {
        name=args.mid(i,l);
        j=args.find(')',i+l)-i-l;
        if (j >= 0) args=args.mid(i+l,j);
      }
      //printf("new: type='%s' name='%s' args='%s'\n",
      //    type.data(),name.data(),args.data());
    }
    else
    {
      int i=isFuncPtr;
      if (i==-1 && (root->spec&Entry::Alias)==0) i=findFunctionPtr(type,root->lang); // for typedefs isFuncPtr is not yet set
      Debug::print(Debug::Variables,0,"  functionPtr? %s\n",i!=-1?"yes":"no");
      if (i!=-1) // function pointer
      {
        int ai = type.find('[',i);
        if (ai>i) // function pointer array
        {
          args.prepend(type.right(type.length()-ai));
          type=type.left(ai);
        }
        else if (type.find(')',i)!=-1) // function ptr, not variable like "int (*bla)[10]"
        {
          type=type.left(type.length()-1);
          args.prepend(") ");
          //printf("type=%s args=%s\n",type.data(),args.data());
        }
      }
    }

    QCString scope;
    name=removeRedundantWhiteSpace(name);

    // find the scope of this variable
    Entry *p = root->parent();
    while ((p->section & Entry::SCOPE_MASK))
    {
      QCString scopeName = p->name;
      if (!scopeName.isEmpty())
      {
        scope.prepend(scopeName);
        break;
      }
      p=p->parent();
    }

    MemberType mtype;
    type=type.stripWhiteSpace();
    ClassDef *cd=0;
    bool isRelated=FALSE;
    bool isMemberOf=FALSE;

    QCString classScope=stripAnonymousNamespaceScope(scope);
    classScope=stripTemplateSpecifiersFromScope(classScope,FALSE);
    QCString annScopePrefix=scope.left(scope.length()-classScope.length());

    if (name.findRev("::")!=-1)
    {
      if (type=="friend class" || type=="friend struct" ||
          type=="friend union")
      {
         cd=getClass(scope);
         if (cd)
         {
           addVariableToClass(root,  // entry
                              cd,    // class to add member to
                              MemberType_Friend, // type of member
                              type,   // type value as string
                              name,   // name of the member
                              args,   // arguments as string
                              FALSE,  // from Anonymous scope
                              0,      // anonymous member
                              Public, // protection
                              Member  // related to a class
                             );
         }
      }
      return;  /* skip this member, because it is a
                * static variable definition (always?), which will be
                * found in a class scope as well, but then we know the
                * correct protection level, so only then it will be
                * inserted in the correct list!
                */
    }

    if (type=="@")
      mtype=MemberType_EnumValue;
    else if (type.left(8)=="typedef ")
      mtype=MemberType_Typedef;
    else if (type.left(7)=="friend ")
      mtype=MemberType_Friend;
    else if (root->mtype==Property)
      mtype=MemberType_Property;
    else if (root->mtype==Event)
      mtype=MemberType_Event;
    else if (type.find("sequence<") != -1)
      mtype=sliceOpt ? MemberType_Sequence : MemberType_Typedef;
    else if (type.find("dictionary<") != -1)
      mtype=sliceOpt ? MemberType_Dictionary : MemberType_Typedef;
    else
      mtype=MemberType_Variable;

    if (!root->relates.isEmpty()) // related variable
    {
      isRelated=TRUE;
      isMemberOf=(root->relatesType == MemberOf);
      if (getClass(root->relates)==0 && !scope.isEmpty())
        scope=mergeScopes(scope,root->relates);
      else
        scope=root->relates;
    }

    cd=getClass(scope);
    if (cd==0 && classScope!=scope) cd=getClass(classScope);
    if (cd)
    {
      MemberDef *md=0;

      // if cd is an anonymous (=tag less) scope we insert the member
      // into a non-anonymous parent scope as well. This is needed to
      // be able to refer to it using \var or \fn

      //int indentDepth=0;
      int si=scope.find('@');
      //int anonyScopes = 0;
      //bool added=FALSE;

      static bool inlineSimpleStructs = Config_getBool(INLINE_SIMPLE_STRUCTS);
      if (si!=-1 && !inlineSimpleStructs) // anonymous scope or type
      {
        QCString pScope;
        ClassDef *pcd=0;
        pScope = scope.left(QMAX(si-2,0)); // scope without tag less parts
        if (!pScope.isEmpty())
          pScope.prepend(annScopePrefix);
        else if (annScopePrefix.length()>2)
          pScope=annScopePrefix.left(annScopePrefix.length()-2);
        if (name.at(0)!='@')
        {
          if (!pScope.isEmpty() && (pcd=getClass(pScope)))
          {
            md=addVariableToClass(root,  // entry
                                  pcd,   // class to add member to
                                  mtype, // member type
                                  type,  // type value as string
                                  name,  // member name
                                  args,  // arguments as string
                                  TRUE,  // from anonymous scope
                                  0,     // from anonymous member
                                  root->protection,
                                  isMemberOf ? Foreign : isRelated ? Related : Member
                                 );
            //added=TRUE;
          }
          else // anonymous scope inside namespace or file => put variable in the global scope
          {
            if (mtype==MemberType_Variable)
            {
              md=addVariableToFile(root,mtype,pScope,type,name,args,TRUE,0);
            }
            //added=TRUE;
          }
        }
      }

      //printf("name='%s' scope=%s scope.right=%s\n",
      //                   name.data(),scope.data(),
      //                   scope.right(scope.length()-si).data());
      addVariableToClass(root,   // entry
                         cd,     // class to add member to
                         mtype,  // member type
                         type,   // type value as string
                         name,   // name of the member
                         args,   // arguments as string
                         FALSE,  // from anonymous scope
                         md,     // from anonymous member
                         root->protection,
                         isMemberOf ? Foreign : isRelated ? Related : Member);
    }
    else if (!name.isEmpty()) // global variable
    {
      //printf("Inserting member in global scope %s!\n",scope.data());
      addVariableToFile(root,mtype,scope,type,name,args,FALSE,/*0,*/0);
    }

}

//----------------------------------------------------------------------
// Searches the Entry tree for typedef documentation sections.
// If found they are stored in their class or in the global list.
static void buildTypedefList(const Entry *root)
{
  //printf("buildVarList(%s)\n",rootNav->name().data());
  if (!root->name.isEmpty() &&
      root->section==Entry::VARIABLE_SEC &&
      root->type.find("typedef ")!=-1 // its a typedef
     )
  {
    addVariable(root);
  }
  for (const auto &e : root->children())
    if (e->section!=Entry::ENUM_SEC)
      buildTypedefList(e.get());
}

//----------------------------------------------------------------------
// Searches the Entry tree for sequence documentation sections.
// If found they are stored in the global list.
static void buildSequenceList(const Entry *root)
{
  if (!root->name.isEmpty() &&
      root->section==Entry::VARIABLE_SEC &&
      root->type.find("sequence<")!=-1 // it's a sequence
     )
  {
    addVariable(root);
  }
  for (const auto &e : root->children())
    if (e->section!=Entry::ENUM_SEC)
      buildSequenceList(e.get());
}

//----------------------------------------------------------------------
// Searches the Entry tree for dictionary documentation sections.
// If found they are stored in the global list.
static void buildDictionaryList(const Entry *root)
{
  if (!root->name.isEmpty() &&
      root->section==Entry::VARIABLE_SEC &&
      root->type.find("dictionary<")!=-1 // it's a dictionary
     )
  {
    addVariable(root);
  }
  for (const auto &e : root->children())
    if (e->section!=Entry::ENUM_SEC)
      buildDictionaryList(e.get());
}

//----------------------------------------------------------------------
// Searches the Entry tree for Variable documentation sections.
// If found they are stored in their class or in the global list.

static void buildVarList(const Entry *root)
{
  //printf("buildVarList(%s) section=%08x\n",rootNav->name().data(),rootNav->section());
  int isFuncPtr=-1;
  if (!root->name.isEmpty() &&
      (root->type.isEmpty() || g_compoundKeywordDict.find(root->type)==0) &&
      (
       (root->section==Entry::VARIABLE_SEC    // it's a variable
       ) ||
       (root->section==Entry::FUNCTION_SEC && // or maybe a function pointer variable
        (isFuncPtr=findFunctionPtr(root->type,root->lang))!=-1
       ) ||
       (root->section==Entry::FUNCTION_SEC && // class variable initialized by constructor
        isVarWithConstructor(root)
       )
      )
     ) // documented variable
  {
    addVariable(root,isFuncPtr);
  }
  for (const auto &e : root->children())
    if (e->section!=Entry::ENUM_SEC)
      buildVarList(e.get());
}

//----------------------------------------------------------------------
// Searches the Entry tree for Interface sections (UNO IDL only).
// If found they are stored in their service or in the global list.
//

static void addInterfaceOrServiceToServiceOrSingleton(
        const Entry *root,
        ClassDef *const cd,
        QCString const& rname)
{
  FileDef *fd = root->fileDef();
  enum MemberType type = (root->section==Entry::EXPORTED_INTERFACE_SEC)
      ? MemberType_Interface
      : MemberType_Service;
  QCString fileName = root->fileName;
  if (fileName.isEmpty() && root->tagInfo())
  {
    fileName = root->tagInfo()->tagName;
  }
  std::unique_ptr<MemberDef> md { createMemberDef(
      fileName, root->startLine, root->startColumn, root->type, rname,
      "", "", root->protection, root->virt, root->stat, Member,
      type, ArgumentList(), root->argList, root->metaData) };
  md->setTagInfo(root->tagInfo());
  md->setMemberClass(cd);
  md->setDocumentation(root->doc,root->docFile,root->docLine);
  md->setDocsForDefinition(false);
  md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
  md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
  md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
  md->setMemberSpecifiers(root->spec);
  md->setMemberGroupId(root->mGrpId);
  md->setTypeConstraints(root->typeConstr);
  md->setLanguage(root->lang);
  md->setBodyDef(fd);
  md->setFileDef(fd);
  md->addSectionsToDefinition(root->anchors);
  QCString const def = root->type + " " + rname;
  md->setDefinition(def);
  md->enableCallGraph(root->callGraph);
  md->enableCallerGraph(root->callerGraph);
  md->enableReferencedByRelation(root->referencedByRelation);
  md->enableReferencesRelation(root->referencesRelation);

  Debug::print(Debug::Functions,0,
      "  Interface Member:\n"
      "    '%s' '%s' proto=%d\n"
      "    def='%s'\n",
      qPrint(root->type),
      qPrint(rname),
      root->proto,
      qPrint(def)
              );


  // add member to the class cd
  cd->insertMember(md.get());
  // also add the member as a "base" (to get nicer diagrams)
  // "optional" interface/service get Protected which turns into dashed line
  BaseInfo base(rname,
          (root->spec & (Entry::Optional)) ? Protected : Public,Normal);
  findClassRelation(root,cd,cd,&base,0,DocumentedOnly,true) || findClassRelation(root,cd,cd,&base,0,Undocumented,true);
  // add file to list of used files
  cd->insertUsedFile(fd);

  addMemberToGroups(root,md.get());
  root->markAsProcessed();
  md->setRefItems(root->sli);

  // add member to the global list of all members
  MemberName *mn = Doxygen::memberNameLinkedMap->add(rname);
  mn->push_back(std::move(md));
}

static void buildInterfaceAndServiceList(const Entry *root)
{
  if (root->section==Entry::EXPORTED_INTERFACE_SEC ||
      root->section==Entry::INCLUDED_SERVICE_SEC)
  {
    Debug::print(Debug::Functions,0,
                 "EXPORTED_INTERFACE_SEC:\n"
                 "  '%s' '%s'::'%s' '%s' relates='%s' relatesType='%d' file='%s' line='%d' bodyLine='%d' #tArgLists=%d mGrpId=%d spec=%lld proto=%d docFile=%s\n",
                 qPrint(root->type),
                 qPrint(root->parent()->name),
                 qPrint(root->name),
                 qPrint(root->args),
                 qPrint(root->relates),
                 root->relatesType,
                 qPrint(root->fileName),
                 root->startLine,
                 root->bodyLine,
                 root->tArgLists.size(),
                 root->mGrpId,
                 root->spec,
                 root->proto,
                 qPrint(root->docFile)
                );

    QCString rname = removeRedundantWhiteSpace(root->name);

    if (!rname.isEmpty())
    {
      QCString scope = root->parent()->name;
      ClassDef *cd = getClass(scope);
      assert(cd);
      if (cd && ((ClassDef::Interface == cd->compoundType()) ||
                 (ClassDef::Service   == cd->compoundType()) ||
                 (ClassDef::Singleton == cd->compoundType())))
      {
        addInterfaceOrServiceToServiceOrSingleton(root,cd,rname);
      }
      else
      {
        assert(false); // was checked by scanner.l
      }
    }
    else if (rname.isEmpty())
    {
      warn(root->fileName,root->startLine,
           "Illegal member name found.");
    }
  }
  // can only have these in IDL anyway
  switch (root->lang)
  {
    case SrcLangExt_Unknown: // fall through (root node always is Unknown)
    case SrcLangExt_IDL:
        for (const auto &e : root->children()) buildInterfaceAndServiceList(e.get());
        break;
    default:
        return; // nothing to do here
  }
}


//----------------------------------------------------------------------
// Searches the Entry tree for Function sections.
// If found they are stored in their class or in the global list.

static void addMethodToClass(const Entry *root,ClassDef *cd,
                  const QCString &rtype,const QCString &rname,const QCString &rargs,
                  bool isFriend,
                  Protection protection,bool stat,Specifier virt,uint64 spec,
                  const QCString &relates
                  )
{
  FileDef *fd=root->fileDef();

  int l;
  static QRegExp re("([a-z_A-Z0-9: ]*[ &*]+[ ]*");
  QCString type = rtype;
  QCString args = rargs;
  int ts=type.find('<');
  int te=type.findRev('>');
  int i=re.match(type,0,&l);
  if (i!=-1 && ts!=-1 && ts<te && ts<i && i<te) // avoid changing A<int(int*)>, see bug 677315
  {
    i=-1;
  }

  if (cd->getLanguage()==SrcLangExt_Cpp && // only C has pointers
      !type.isEmpty() && (root->spec&Entry::Alias)==0 && i!=-1) // function variable
  {
    args+=type.right(type.length()-i-l);
    type=type.left(i+l);
  }

  QCString name=removeRedundantWhiteSpace(rname);
  if (name.left(2)=="::") name=name.right(name.length()-2);

  MemberType mtype;
  if (isFriend)                 mtype=MemberType_Friend;
  else if (root->mtype==Signal) mtype=MemberType_Signal;
  else if (root->mtype==Slot)   mtype=MemberType_Slot;
  else if (root->mtype==DCOP)   mtype=MemberType_DCOP;
  else                          mtype=MemberType_Function;

  // strip redundant template specifier for constructors
  int j = -1;
  if ((fd==0 || fd->getLanguage()==SrcLangExt_Cpp) &&
      name.left(9)!="operator " &&   // not operator
      (i=name.find('<'))!=-1    &&   // containing <
      (j=name.find('>'))!=-1    &&   // or >
      (j!=i+2 || name.at(i+1)!='=')  // but not the C++20 spaceship operator <=>
     )
  {
    name=name.left(i);
  }

  QCString fileName = root->fileName;
  if (fileName.isEmpty() && root->tagInfo())
  {
    fileName = root->tagInfo()->tagName;
  }

  //printf("root->name='%s; args='%s' root->argList='%s'\n",
  //    root->name.data(),args.data(),argListToString(root->argList).data()
  //   );

  // adding class member
  std::unique_ptr<MemberDef> md { createMemberDef(
      fileName,root->startLine,root->startColumn,
      type,name,args,root->exception,
      protection,virt,
      stat && root->relatesType != MemberOf,
      relates.isEmpty() ? Member :
          root->relatesType == MemberOf ? Foreign : Related,
      mtype,!root->tArgLists.empty() ? root->tArgLists.back() : ArgumentList(),
      root->argList, root->metaData) };
  md->setTagInfo(root->tagInfo());
  md->setMemberClass(cd);
  md->setDocumentation(root->doc,root->docFile,root->docLine);
  md->setDocsForDefinition(!root->proto);
  md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
  md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
  md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
  md->setMemberSpecifiers(spec);
  md->setMemberGroupId(root->mGrpId);
  md->setTypeConstraints(root->typeConstr);
  md->setLanguage(root->lang);
  md->setId(root->id);
  md->setBodyDef(fd);
  md->setFileDef(fd);
  md->addSectionsToDefinition(root->anchors);
  QCString def;
  QCString qualScope = cd->qualifiedNameWithTemplateParameters();
  SrcLangExt lang = cd->getLanguage();
  QCString scopeSeparator=getLanguageSpecificSeparator(lang);
  if (scopeSeparator!="::")
  {
    qualScope = substitute(qualScope,"::",scopeSeparator);
  }
  if (lang==SrcLangExt_PHP)
  {
    // for PHP we use Class::method and Namespace\method
    scopeSeparator="::";
  }
//  QCString optArgs = root->argList.empty() ? args : QCString();
  if (!relates.isEmpty() || isFriend || Config_getBool(HIDE_SCOPE_NAMES))
  {
    if (!type.isEmpty())
    {
      def=type+" "+name; //+optArgs;
    }
    else
    {
      def=name; //+optArgs;
    }
  }
  else
  {
    if (!type.isEmpty())
    {
      def=type+" "+qualScope+scopeSeparator+name; //+optArgs;
    }
    else
    {
      def=qualScope+scopeSeparator+name; //+optArgs;
    }
  }
  if (def.left(7)=="friend ") def=def.right(def.length()-7);
  md->setDefinition(def);
  md->enableCallGraph(root->callGraph);
  md->enableCallerGraph(root->callerGraph);
  md->enableReferencedByRelation(root->referencedByRelation);
  md->enableReferencesRelation(root->referencesRelation);

  Debug::print(Debug::Functions,0,
      "  Func Member:\n"
      "    '%s' '%s'::'%s' '%s' proto=%d\n"
      "    def='%s'\n",
      qPrint(type),
      qPrint(qualScope),
      qPrint(rname),
      qPrint(args),
      root->proto,
      qPrint(def)
              );

  // add member to the class cd
  cd->insertMember(md.get());
  // add file to list of used files
  cd->insertUsedFile(fd);

  addMemberToGroups(root,md.get());
  root->markAsProcessed();
  md->setRefItems(root->sli);

  // add member to the global list of all members
  //printf("Adding member=%s class=%s\n",md->name().data(),cd->name().data());
  MemberName *mn = Doxygen::memberNameLinkedMap->add(name);
  mn->push_back(std::move(md));
}

//------------------------------------------------------------------------------------------

void addGlobalFunction(const Entry *root,const QCString &rname,const QCString &sc,
                       NamespaceDef *nd)
{
  QCString scope = sc;
  Debug::print(Debug::Functions,0,"  --> new function %s found!\n",qPrint(rname));
  //printf("New function type='%s' name='%s' args='%s' bodyLine=%d\n",
  //       root->type.data(),rname.data(),root->args.data(),root->bodyLine);

  // new global function
  QCString name=removeRedundantWhiteSpace(rname);
  std::unique_ptr<MemberDef> md { createMemberDef(
      root->fileName,root->startLine,root->startColumn,
      root->type,name,root->args,root->exception,
      root->protection,root->virt,root->stat,Member,
      MemberType_Function,
      !root->tArgLists.empty() ? root->tArgLists.back() : ArgumentList(),
      root->argList,root->metaData) };

  md->setTagInfo(root->tagInfo());
  md->setLanguage(root->lang);
  md->setId(root->id);
  md->setDocumentation(root->doc,root->docFile,root->docLine);
  md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
  md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
  md->setPrototype(root->proto,root->fileName,root->startLine,root->startColumn);
  md->setDocsForDefinition(!root->proto);
  md->setTypeConstraints(root->typeConstr);
  //md->setBody(root->body);
  md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
  FileDef *fd=root->fileDef();
  md->setBodyDef(fd);
  md->addSectionsToDefinition(root->anchors);
  md->setMemberSpecifiers(root->spec);
  md->setMemberGroupId(root->mGrpId);

  // see if the function is inside a namespace that was not part of
  // the name already (in that case nd should be non-zero already)
  if (nd==0 && root->parent()->section == Entry::NAMESPACE_SEC )
  {
    //QCString nscope=removeAnonymousScopes(root->parent()->name);
    QCString nscope=root->parent()->name;
    if (!nscope.isEmpty())
    {
      nd = getResolvedNamespace(nscope);
    }
  }

  if (!scope.isEmpty())
  {
    QCString sep = getLanguageSpecificSeparator(root->lang);
    if (sep!="::")
    {
      scope = substitute(scope,"::",sep);
    }
    scope+=sep;
  }

  QCString def;
  //QCString optArgs = root->argList.empty() ? QCString() : root->args;
  if (!root->type.isEmpty())
  {
    def=root->type+" "+scope+name; //+optArgs;
  }
  else
  {
    def=scope+name; //+optArgs;
  }
  Debug::print(Debug::Functions,0,
      "  Global Function:\n"
      "    '%s' '%s'::'%s' '%s' proto=%d\n"
      "    def='%s'\n",
      qPrint(root->type),
      qPrint(root->parent()->name),
      qPrint(rname),
      qPrint(root->args),
      root->proto,
      qPrint(def)
      );
  md->setDefinition(def);
  md->enableCallGraph(root->callGraph);
  md->enableCallerGraph(root->callerGraph);
  md->enableReferencedByRelation(root->referencedByRelation);
  md->enableReferencesRelation(root->referencesRelation);
  //if (root->mGrpId!=-1)
  //{
  //  md->setMemberGroup(memberGroupDict[root->mGrpId]);
  //}

  md->setRefItems(root->sli);
  if (nd && !nd->name().isEmpty() && nd->name().at(0)!='@')
  {
    // add member to namespace
    md->setNamespace(nd);
    nd->insertMember(md.get());
  }
  if (fd)
  {
    // add member to the file (we do this even if we have already
    // inserted it into the namespace)
    md->setFileDef(fd);
    fd->insertMember(md.get());
  }

  addMemberToGroups(root,md.get());
  if (root->relatesType == Simple) // if this is a relatesalso command,
                                   // allow find Member to pick it up
  {
    root->markAsProcessed(); // Otherwise we have finished with this entry.
  }

  // add member to the list of file members
  //printf("Adding member=%s\n",md->name().data());
  MemberName *mn = Doxygen::functionNameLinkedMap->add(name);
  mn->push_back(std::move(md));
}

//------------------------------------------------------------------------------------------

static void buildFunctionList(const Entry *root)
{
  if (root->section==Entry::FUNCTION_SEC)
  {
    Debug::print(Debug::Functions,0,
                 "FUNCTION_SEC:\n"
                 "  '%s' '%s'::'%s' '%s' relates='%s' relatesType='%d' file='%s' line='%d' bodyLine='%d' #tArgLists=%d mGrpId=%d spec=%lld proto=%d docFile=%s\n",
                 qPrint(root->type),
                 qPrint(root->parent()->name),
                 qPrint(root->name),
                 qPrint(root->args),
                 qPrint(root->relates),
                 root->relatesType,
                 qPrint(root->fileName),
                 root->startLine,
                 root->bodyLine,
                 root->tArgLists.size(),
                 root->mGrpId,
                 root->spec,
                 root->proto,
                 qPrint(root->docFile)
                );

    bool isFriend=root->type.find("friend ")!=-1;
    QCString rname = removeRedundantWhiteSpace(root->name);
    //printf("rname=%s\n",rname.data());

    QCString scope=root->parent()->name; //stripAnonymousNamespaceScope(root->parent->name);
    if (!rname.isEmpty() && scope.find('@')==-1)
    {
      ClassDef *cd=0;
      // check if this function's parent is a class
      scope=stripTemplateSpecifiersFromScope(scope,FALSE);

      FileDef *rfd=root->fileDef();

      int memIndex=rname.findRev("::");

      cd=getClass(scope);
      if (cd && scope+"::"==rname.left(scope.length()+2)) // found A::f inside A
      {
        // strip scope from name
        rname=rname.right(rname.length()-root->parent()->name.length()-2);
      }

      NamespaceDef *nd = 0;
      bool isMember=FALSE;
      if (memIndex!=-1)
      {
        int ts=rname.find('<');
        int te=rname.find('>');
        if (memIndex>0 && (ts==-1 || te==-1))
        {
          // note: the following code was replaced by inMember=TRUE to deal with a
          // function rname='X::foo' of class X inside a namespace also called X...
          // bug id 548175
          //nd = Doxygen::namespaceSDict->find(rname.left(memIndex));
          //isMember = nd==0;
          //if (nd)
          //{
          //  // strip namespace scope from name
          //  scope=rname.left(memIndex);
          //  rname=rname.right(rname.length()-memIndex-2);
          //}
          isMember = TRUE;
        }
        else
        {
          isMember=memIndex<ts || memIndex>te;
        }
      }

      static QRegExp re("([a-z_A-Z0-9: ]*[ &*]+[ ]*");
      int ts=root->type.find('<');
      int te=root->type.findRev('>');
      int ti;
      if (!root->parent()->name.isEmpty() &&
          (root->parent()->section & Entry::COMPOUND_MASK) &&
          cd &&
          // do some fuzzy things to exclude function pointers
          (root->type.isEmpty() ||
           ((ti=root->type.find(re,0))==-1 ||      // type does not contain ..(..*
            (ts!=-1 && ts<te && ts<ti && ti<te) || // outside of < ... >
           root->args.find(")[")!=-1) ||           // and args not )[.. -> function pointer
           root->type.find(")(")!=-1 || root->type.find("operator")!=-1 || // type contains ..)(.. and not "operator"
           cd->getLanguage()!=SrcLangExt_Cpp                               // language other than C
          )
         )
      {
        Debug::print(Debug::Functions,0,"  --> member %s of class %s!\n",
            qPrint(rname),qPrint(cd->name()));
        addMethodToClass(root,cd,root->type,rname,root->args,isFriend,
                         root->protection,root->stat,root->virt,root->spec,root->relates);
      }
      else if (!((root->parent()->section & Entry::COMPOUND_MASK)
                 || root->parent()->section==Entry::OBJCIMPL_SEC
                ) &&
               !isMember &&
               (root->relates.isEmpty() || root->relatesType == Duplicate) &&
               root->type.left(7)!="extern " && root->type.left(8)!="typedef "
              )
      // no member => unrelated function
      {
        /* check the uniqueness of the function name in the file.
         * A file could contain a function prototype and a function definition
         * or even multiple function prototypes.
         */
        bool found=FALSE;
        MemberName *mn;
        MemberDef *md_found=0;
        if ((mn=Doxygen::functionNameLinkedMap->find(rname)))
        {
          Debug::print(Debug::Functions,0,"  --> function %s already found!\n",qPrint(rname));
          for (const auto &md : *mn)
          {
            if (!md->isAlias())
            {
              const NamespaceDef *mnd = md->getNamespaceDef();
              NamespaceDef *rnd = 0;
              //printf("root namespace=%s\n",rootNav->parent()->name().data());
              QCString fullScope = scope;
              QCString parentScope = root->parent()->name;
              if (!parentScope.isEmpty() && !leftScopeMatch(parentScope,scope))
              {
                if (!scope.isEmpty()) fullScope.prepend("::");
                fullScope.prepend(parentScope);
              }
              //printf("fullScope=%s\n",fullScope.data());
              rnd = getResolvedNamespace(fullScope);
              const FileDef *mfd = md->getFileDef();
              QCString nsName,rnsName;
              if (mnd)  nsName = mnd->name().copy();
              if (rnd) rnsName = rnd->name().copy();
              //printf("matching arguments for %s%s %s%s\n",
              //    md->name().data(),md->argsString(),rname.data(),argListToString(root->argList).data());
              ArgumentList &mdAl = md->argumentList();
              const ArgumentList &mdTempl = md->templateArguments();

              // in case of template functions, we need to check if the
              // functions have the same number of template parameters
              bool sameNumTemplateArgs = TRUE;
              bool matchingReturnTypes = TRUE;
              if (!mdTempl.empty() && !root->tArgLists.empty())
              {
                if (mdTempl.size()!=root->tArgLists.back().size())
                {
                  sameNumTemplateArgs = FALSE;
                }
                if (md->typeString()!=removeRedundantWhiteSpace(root->type))
                {
                  matchingReturnTypes = FALSE;
                }
              }

              bool staticsInDifferentFiles =
                root->stat && md->isStatic() && root->fileName!=md->getDefFileName();

              if (
                  matchArguments2(md->getOuterScope(),mfd,&mdAl,
                    rnd ? rnd : Doxygen::globalScope,rfd,&root->argList,
                    FALSE) &&
                  sameNumTemplateArgs &&
                  matchingReturnTypes &&
                  !staticsInDifferentFiles
                 )
              {
                GroupDef *gd=0;
                if (!root->groups.empty() && !root->groups.front().groupname.isEmpty())
                {
                  gd = Doxygen::groupSDict->find(root->groups.front().groupname);
                }
                //printf("match!\n");
                //printf("mnd=%p rnd=%p nsName=%s rnsName=%s\n",mnd,rnd,nsName.data(),rnsName.data());
                // see if we need to create a new member
                found=(mnd && rnd && nsName==rnsName) ||   // members are in the same namespace
                  ((mnd==0 && rnd==0 && mfd!=0 &&       // no external reference and
                    mfd->absFilePath()==root->fileName // prototype in the same file
                   )
                  );
                // otherwise, allow a duplicate global member with the same argument list
                if (!found && gd && gd==md->getGroupDef() && nsName==rnsName)
                {
                  // member is already in the group, so we don't want to add it again.
                  found=TRUE;
                }

                //printf("combining function with prototype found=%d in namespace %s\n",
                //    found,nsName.data());

                if (found)
                {
                  // merge argument lists
                  ArgumentList mergedArgList = root->argList;
                  mergeArguments(mdAl,mergedArgList,!root->doc.isEmpty());
                  // merge documentation
                  if (md->documentation().isEmpty() && !root->doc.isEmpty())
                  {
                    if (root->proto)
                    {
                      //printf("setDeclArgumentList to %p\n",argList);
                      md->moveDeclArgumentList(stringToArgumentList(root->lang,root->args));
                    }
                    else
                    {
                      md->moveArgumentList(stringToArgumentList(root->lang,root->args));
                    }
                  }

                  md->setDocumentation(root->doc,root->docFile,root->docLine);
                  md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
                  md->setDocsForDefinition(!root->proto);
                  if (md->getStartBodyLine()==-1 && root->bodyLine!=-1)
                  {
                    md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
                    md->setBodyDef(rfd);
                  }

                  if (md->briefDescription().isEmpty() && !root->brief.isEmpty())
                  {
                    md->setArgsString(root->args);
                  }
                  md->setBriefDescription(root->brief,root->briefFile,root->briefLine);

                  md->addSectionsToDefinition(root->anchors);

                  md->enableCallGraph(md->hasCallGraph() || root->callGraph);
                  md->enableCallerGraph(md->hasCallerGraph() || root->callerGraph);
                  md->enableReferencedByRelation(md->hasReferencedByRelation() || root->referencedByRelation);
                  md->enableReferencesRelation(md->hasReferencesRelation() || root->referencesRelation);

                  // merge ingroup specifiers
                  if (md->getGroupDef()==0 && !root->groups.empty())
                  {
                    addMemberToGroups(root,md.get());
                  }
                  else if (md->getGroupDef()!=0 && root->groups.empty())
                  {
                    //printf("existing member is grouped, new member not\n");
                  }
                  else if (md->getGroupDef()!=0 && !root->groups.empty())
                  {
                    //printf("both members are grouped\n");
                  }

                  // if md is a declaration and root is the corresponding
                  // definition, then turn md into a definition.
                  if (md->isPrototype() && !root->proto)
                  {
                    md->setDeclFile(md->getDefFileName(),md->getDefLine(),md->getDefColumn());
                    md->setPrototype(FALSE,root->fileName,root->startLine,root->startColumn);
                  }
                  // if md is already the definition, then add the declaration info
                  else if (!md->isPrototype() && root->proto)
                  {
                    md->setDeclFile(root->fileName,root->startLine,root->startColumn);
                  }
                }
              }
            }
            if (found)
            {
              md_found = md.get();
              break;
            }
          }
        }
        if (!found) /* global function is unique with respect to the file */
        {
          addGlobalFunction(root,rname,scope,nd);
        }
        else
        {
          FileDef *fd=root->fileDef();
          if (fd)
          {
            // add member to the file (we do this even if we have already
            // inserted it into the namespace)
            fd->insertMember(md_found);
          }
        }

        //printf("unrelated function %d '%s' '%s' '%s'\n",
        //    root->parent->section,root->type.data(),rname.data(),root->args.data());
      }
      else
      {
          Debug::print(Debug::Functions,0,"  --> %s not processed!\n",qPrint(rname));
      }
    }
    else if (rname.isEmpty())
    {
        warn(root->fileName,root->startLine,
             "Illegal member name found."
            );
    }
  }
  for (const auto &e : root->children()) buildFunctionList(e.get());
}

//----------------------------------------------------------------------

static void findFriends()
{
  //printf("findFriends()\n");
  for (const auto &fn : *Doxygen::functionNameLinkedMap) // for each global function name
  {
    //printf("Function name='%s'\n",fn->memberName());
    MemberName *mn;
    if ((mn=Doxygen::memberNameLinkedMap->find(fn->memberName())))
    { // there are members with the same name
      //printf("Function name is also a member name\n");
      // for each function with that name
      for (const auto &fmd : *fn)
      {
        const MemberDef *cfmd = fmd.get();
        // for each member with that name
        for (const auto &mmd : *mn)
        {
          const MemberDef *cmmd = mmd.get();
          //printf("Checking for matching arguments
          //        mmd->isRelated()=%d mmd->isFriend()=%d mmd->isFunction()=%d\n",
          //    mmd->isRelated(),mmd->isFriend(),mmd->isFunction());
          if ((cmmd->isFriend() || (cmmd->isRelated() && cmmd->isFunction())) &&
              !fmd->isAlias() && !mmd->isAlias() &&
              matchArguments2(cmmd->getOuterScope(), cmmd->getFileDef(), &cmmd->argumentList(),
                              cfmd->getOuterScope(), cfmd->getFileDef(), &cfmd->argumentList(),
                              TRUE
                             )

             ) // if the member is related and the arguments match then the
               // function is actually a friend.
          {
            ArgumentList &mmdAl = mmd->argumentList();
            ArgumentList &fmdAl = fmd->argumentList();
            mergeArguments(mmdAl,fmdAl);
            if (!fmd->documentation().isEmpty())
            {
              mmd->setDocumentation(fmd->documentation(),fmd->docFile(),fmd->docLine());
            }
            else if (!mmd->documentation().isEmpty())
            {
              fmd->setDocumentation(mmd->documentation(),mmd->docFile(),mmd->docLine());
            }
            if (mmd->briefDescription().isEmpty() && !fmd->briefDescription().isEmpty())
            {
              mmd->setBriefDescription(fmd->briefDescription(),fmd->briefFile(),fmd->briefLine());
            }
            else if (!mmd->briefDescription().isEmpty() && !fmd->briefDescription().isEmpty())
            {
              fmd->setBriefDescription(mmd->briefDescription(),mmd->briefFile(),mmd->briefLine());
            }
            if (!fmd->inbodyDocumentation().isEmpty())
            {
              mmd->setInbodyDocumentation(fmd->inbodyDocumentation(),fmd->inbodyFile(),fmd->inbodyLine());
            }
            else if (!mmd->inbodyDocumentation().isEmpty())
            {
              fmd->setInbodyDocumentation(mmd->inbodyDocumentation(),mmd->inbodyFile(),mmd->inbodyLine());
            }
            //printf("body mmd %d fmd %d\n",mmd->getStartBodyLine(),fmd->getStartBodyLine());
            if (mmd->getStartBodyLine()==-1 && fmd->getStartBodyLine()!=-1)
            {
              mmd->setBodySegment(fmd->getDefLine(),fmd->getStartBodyLine(),fmd->getEndBodyLine());
              mmd->setBodyDef(fmd->getBodyDef());
              //mmd->setBodyMember(fmd);
            }
            else if (mmd->getStartBodyLine()!=-1 && fmd->getStartBodyLine()==-1)
            {
              fmd->setBodySegment(mmd->getDefLine(),mmd->getStartBodyLine(),mmd->getEndBodyLine());
              fmd->setBodyDef(mmd->getBodyDef());
              //fmd->setBodyMember(mmd);
            }
            mmd->setDocsForDefinition(fmd->isDocsForDefinition());

            mmd->enableCallGraph(mmd->hasCallGraph() || fmd->hasCallGraph());
            mmd->enableCallerGraph(mmd->hasCallerGraph() || fmd->hasCallerGraph());
            mmd->enableReferencedByRelation(mmd->hasReferencedByRelation() || fmd->hasReferencedByRelation());
            mmd->enableReferencesRelation(mmd->hasReferencesRelation() || fmd->hasReferencesRelation());

            fmd->enableCallGraph(mmd->hasCallGraph() || fmd->hasCallGraph());
            fmd->enableCallerGraph(mmd->hasCallerGraph() || fmd->hasCallerGraph());
            fmd->enableReferencedByRelation(mmd->hasReferencedByRelation() || fmd->hasReferencedByRelation());
            fmd->enableReferencesRelation(mmd->hasReferencesRelation() || fmd->hasReferencesRelation());
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------

static void transferFunctionDocumentation()
{
  //printf("---- transferFunctionDocumentation()\n");

  // find matching function declaration and definitions.
  for (const auto &mn : *Doxygen::functionNameLinkedMap)
  {
    //printf("memberName=%s count=%d\n",mn->memberName(),mn->count());
    /* find a matching function declaration and definition for this function */
    for (const auto &mdec : *mn)
    {
      if (mdec->isPrototype() ||
          (mdec->isVariable() && mdec->isExternal())
         )
      {
        for (const auto &mdef : *mn)
        {
          if (mdec!=mdef &&
              !mdec->isAlias() && !mdef->isAlias() &&
              mdec->getNamespaceDef()==mdef->getNamespaceDef())
          {
            combineDeclarationAndDefinition(mdec.get(),mdef.get());
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------

static void transferFunctionReferences()
{
  for (const auto &mn : *Doxygen::functionNameLinkedMap)
  {
    MemberDef *mdef=0,*mdec=0;
    /* find a matching function declaration and definition for this function */
    for (const auto &md_p : *mn)
    {
      MemberDef *md = md_p.get();
      if (md->isPrototype())
        mdec=md;
      else if (md->isVariable() && md->isExternal())
        mdec=md;

      if (md->isFunction() && !md->isStatic() && !md->isPrototype())
        mdef=md;
      else if (md->isVariable() && !md->isExternal() && !md->isStatic())
        mdef=md;

      if (mdef && mdec) break;
    }
    if (mdef && mdec)
    {
      ArgumentList &mdefAl = mdef->argumentList();
      ArgumentList &mdecAl = mdec->argumentList();
      if (
          matchArguments2(mdef->getOuterScope(),mdef->getFileDef(),&mdefAl,
                          mdec->getOuterScope(),mdec->getFileDef(),&mdecAl,
                          TRUE
            )
         ) /* match found */
      {
        MemberSDict *defDict = mdef->getReferencesMembers();
        MemberSDict *decDict = mdec->getReferencesMembers();
        if (defDict!=0)
        {
          MemberSDict::IteratorDict msdi(*defDict);
          MemberDef *rmd;
          for (msdi.toFirst();(rmd=msdi.current());++msdi)
          {
            if (decDict==0 || decDict->find(rmd->name())==0)
            {
              mdec->addSourceReferences(rmd);
            }
          }
        }
        if (decDict!=0)
        {
          MemberSDict::IteratorDict msdi(*decDict);
          MemberDef *rmd;
          for (msdi.toFirst();(rmd=msdi.current());++msdi)
          {
            if (defDict==0 || defDict->find(rmd->name())==0)
            {
              mdef->addSourceReferences(rmd);
            }
          }
        }

        defDict = mdef->getReferencedByMembers();
        decDict = mdec->getReferencedByMembers();
        if (defDict!=0)
        {
          MemberSDict::IteratorDict msdi(*defDict);
          MemberDef *rmd;
          for (msdi.toFirst();(rmd=msdi.current());++msdi)
          {
            if (decDict==0 || decDict->find(rmd->name())==0)
            {
              mdec->addSourceReferencedBy(rmd);
            }
          }
        }
        if (decDict!=0)
        {
          MemberSDict::IteratorDict msdi(*decDict);
          MemberDef *rmd;
          for (msdi.toFirst();(rmd=msdi.current());++msdi)
          {
            if (defDict==0 || defDict->find(rmd->name())==0)
            {
              mdef->addSourceReferencedBy(rmd);
            }
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------

static void transferRelatedFunctionDocumentation()
{
  // find match between function declaration and definition for
  // related functions
  for (const auto &mn : *Doxygen::functionNameLinkedMap)
  {
    /* find a matching function declaration and definition for this function */
    // for each global function
    for (const auto &md : *mn)
    {
      //printf("  Function '%s'\n",md->name().data());
      MemberName *rmn;
      if ((rmn=Doxygen::memberNameLinkedMap->find(md->name()))) // check if there is a member with the same name
      {
        //printf("  Member name found\n");
        // for each member with the same name
        for (const auto &rmd : *rmn)
        {
          //printf("  Member found: related='%d'\n",rmd->isRelated());
          if ((rmd->isRelated() || rmd->isForeign()) && // related function
              !md->isAlias() && !rmd->isAlias() &&
              matchArguments2( md->getOuterScope(), md->getFileDef(), &md->argumentList(),
                              rmd->getOuterScope(),rmd->getFileDef(),&rmd->argumentList(),
                              TRUE
                             )
             )
          {
            //printf("  Found related member '%s'\n",md->name().data());
            if (rmd->relatedAlso())
              md->setRelatedAlso(rmd->relatedAlso());
            else if (rmd->isForeign())
              md->makeForeign();
            else
              md->makeRelated();
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------

/*! make a dictionary of all template arguments of class cd
 * that are part of the base class name.
 * Example: A template class A with template arguments <R,S,T>
 * that inherits from B<T,T,S> will have T and S in the dictionary.
 */
static QDict<int> *getTemplateArgumentsInName(const ArgumentList &templateArguments,const QCString &name)
{
  QDict<int> *templateNames = new QDict<int>(17);
  templateNames->setAutoDelete(TRUE);
  static QRegExp re("[a-z_A-Z][a-z_A-Z0-9:]*");
  int count=0;
  for (const Argument &arg : templateArguments)
  {
    int i,p=0,l;
    while ((i=re.match(name,p,&l))!=-1)
    {
      QCString n = name.mid(i,l);
      if (n==arg.name)
      {
        if (templateNames->find(n)==0)
        {
          templateNames->insert(n,new int(count));
        }
      }
      p=i+l;
    }
  }
  return templateNames;
}

/*! Searches a class from within \a context and \a cd and returns its
 *  definition if found (otherwise 0 is returned).
 */
static ClassDef *findClassWithinClassContext(Definition *context,ClassDef *cd,const QCString &name)
{
  ClassDef *result=0;
  if (cd==0)
  {
    return result;
  }
  FileDef *fd=cd->getFileDef();
  if (context && cd!=context)
  {
    result = const_cast<ClassDef*>(getResolvedClass(context,0,name,0,0,TRUE,TRUE));
  }
  if (result==0)
  {
    result = const_cast<ClassDef*>(getResolvedClass(cd,fd,name,0,0,TRUE,TRUE));
  }
  if (result==0) // try direct class, needed for namespaced classes imported via tag files (see bug624095)
  {
    result = getClass(name);
  }
  if (result==0 &&
      (cd->getLanguage()==SrcLangExt_CSharp || cd->getLanguage()==SrcLangExt_Java) &&
      name.find('<')!=-1)
  {
    result = Doxygen::genericsDict->find(name);
  }
  //printf("** Trying to find %s within context %s class %s result=%s lookup=%p\n",
  //       name.data(),
  //       context ? context->name().data() : "<none>",
  //       cd      ? cd->name().data()      : "<none>",
  //       result  ? result->name().data()  : "<none>",
  //       Doxygen::classSDict->find(name)
  //      );
  return result;
}


static void findUsedClassesForClass(const Entry *root,
                           Definition *context,
                           ClassDef *masterCd,
                           ClassDef *instanceCd,
                           bool isArtificial,
                           const std::unique_ptr<ArgumentList> &actualArgs = std::unique_ptr<ArgumentList>(),
                           QDict<int> *templateNames=0
                           )
{
  masterCd->setVisited(TRUE);
  const ArgumentList &formalArgs = masterCd->templateArguments();
  for (auto &mni : masterCd->memberNameInfoLinkedMap())
  {
    for (auto &mi : *mni)
    {
      MemberDef *md=mi->memberDef();
      if (md->isVariable() || md->isObjCProperty()) // for each member variable in this class
      {
        //printf("    Found variable %s in class %s\n",md->name().data(),masterCd->name().data());
        QCString type = normalizeNonTemplateArgumentsInString(md->typeString(),masterCd,formalArgs);
        QCString typedefValue = resolveTypeDef(masterCd,type);
        if (!typedefValue.isEmpty())
        {
          type = typedefValue;
        }
        int pos=0;
        QCString usedClassName;
        QCString templSpec;
        bool found=FALSE;
        // the type can contain template variables, replace them if present
        type = substituteTemplateArgumentsInString(type,formalArgs,actualArgs);

        //printf("      template substitution gives=%s\n",type.data());
        while (!found && extractClassNameFromType(type,pos,usedClassName,templSpec,root->lang)!=-1)
        {
          // find the type (if any) that matches usedClassName
          const ClassDef *typeCd = getResolvedClass(masterCd,
              masterCd->getFileDef(),
              usedClassName,
              0,0,
              FALSE,TRUE
              );
          //printf("====>  usedClassName=%s -> typeCd=%s\n",
          //     usedClassName.data(),typeCd?typeCd->name().data():"<none>");
          if (typeCd)
          {
            usedClassName = typeCd->name();
          }

          int sp=usedClassName.find('<');
          if (sp==-1) sp=0;
          int si=usedClassName.findRev("::",sp);
          if (si!=-1)
          {
            // replace any namespace aliases
            replaceNamespaceAliases(usedClassName,si);
          }
          // add any template arguments to the class
          QCString usedName = removeRedundantWhiteSpace(usedClassName+templSpec);
          //printf("    usedName=%s\n",usedName.data());

          bool delTempNames=FALSE;
          if (templateNames==0)
          {
            templateNames = getTemplateArgumentsInName(formalArgs,usedName);
            delTempNames=TRUE;
          }
          BaseInfo bi(usedName,Public,Normal);
          findClassRelation(root,context,instanceCd,&bi,templateNames,TemplateInstances,isArtificial);

          for (const Argument &arg : masterCd->templateArguments())
          {
            if (arg.name==usedName) // type is a template argument
            {
              found=TRUE;
              Debug::print(Debug::Classes,0,"    New used class '%s'\n", qPrint(usedName));

              ClassDef *usedCd = Doxygen::hiddenClasses->find(usedName);
              if (usedCd==0)
              {
                usedCd = createClassDef(
                    masterCd->getDefFileName(),masterCd->getDefLine(),
                    masterCd->getDefColumn(),
                    usedName,
                    ClassDef::Class);
                //printf("making %s a template argument!!!\n",usedCd->name().data());
                usedCd->makeTemplateArgument();
                usedCd->setUsedOnly(TRUE);
                usedCd->setLanguage(masterCd->getLanguage());
                Doxygen::hiddenClasses->append(usedName,usedCd);
              }
              if (isArtificial) usedCd->setArtificial(TRUE);
              Debug::print(Debug::Classes,0,"      Adding used class '%s' (1)\n", qPrint(usedCd->name()));
              instanceCd->addUsedClass(usedCd,md->name(),md->protection());
              usedCd->addUsedByClass(instanceCd,md->name(),md->protection());
            }
          }

          if (!found)
          {
            ClassDef *usedCd=findClassWithinClassContext(context,masterCd,usedName);
            //printf("Looking for used class %s: result=%s master=%s\n",
            //    usedName.data(),usedCd?usedCd->name().data():"<none>",masterCd?masterCd->name().data():"<none>");

            if (usedCd)
            {
              found=TRUE;
              Debug::print(Debug::Classes,0,"    Adding used class '%s' (2)\n", qPrint(usedCd->name()));
              instanceCd->addUsedClass(usedCd,md->name(),md->protection()); // class exists
              usedCd->addUsedByClass(instanceCd,md->name(),md->protection());
            }
          }
          if (delTempNames)
          {
            delete templateNames;
            templateNames=0;
          }
        }
        if (!found && !type.isEmpty()) // used class is not documented in any scope
        {
          ClassDef *usedCd = Doxygen::hiddenClasses->find(type);
          if (usedCd==0 && !Config_getBool(HIDE_UNDOC_RELATIONS))
          {
            if (type.right(2)=="(*" || type.right(2)=="(^") // type is a function pointer
            {
              type+=md->argsString();
            }
            Debug::print(Debug::Classes,0,"  New undocumented used class '%s'\n", qPrint(type));
            usedCd = createClassDef(
                masterCd->getDefFileName(),masterCd->getDefLine(),
                masterCd->getDefColumn(),
                type,ClassDef::Class);
            usedCd->setUsedOnly(TRUE);
            usedCd->setLanguage(masterCd->getLanguage());
            Doxygen::hiddenClasses->append(type,usedCd);
          }
          if (usedCd)
          {
            if (isArtificial) usedCd->setArtificial(TRUE);
            Debug::print(Debug::Classes,0,"    Adding used class '%s' (3)\n", qPrint(usedCd->name()));
            instanceCd->addUsedClass(usedCd,md->name(),md->protection());
            usedCd->addUsedByClass(instanceCd,md->name(),md->protection());
          }
        }
      }
    }
  }
}

static void findBaseClassesForClass(
      const Entry *root,
      Definition *context,
      ClassDef *masterCd,
      ClassDef *instanceCd,
      FindBaseClassRelation_Mode mode,
      bool isArtificial,
      const std::unique_ptr<ArgumentList> &actualArgs = std::unique_ptr<ArgumentList>(),
      QDict<int> *templateNames=0
    )
{
  //if (masterCd->visited) return;
  masterCd->setVisited(TRUE);
  // The base class could ofcouse also be a non-nested class
  const ArgumentList &formalArgs = masterCd->templateArguments();
  for (const BaseInfo &bi : root->extends)
  {
    //printf("masterCd=%s bi->name='%s' #actualArgs=%d\n",
    //    masterCd->localName().data(),bi->name.data(),actualArgs?(int)actualArgs->count():-1);
    bool delTempNames=FALSE;
    if (templateNames==0)
    {
      templateNames = getTemplateArgumentsInName(formalArgs,bi.name);
      delTempNames=TRUE;
    }
    BaseInfo tbi = bi;
    tbi.name = substituteTemplateArgumentsInString(bi.name,formalArgs,actualArgs);
    //printf("bi->name=%s tbi.name=%s\n",bi->name.data(),tbi.name.data());

    if (mode==DocumentedOnly)
    {
      // find a documented base class in the correct scope
      if (!findClassRelation(root,context,instanceCd,&tbi,templateNames,DocumentedOnly,isArtificial))
      {
        // 1.8.2: decided to show inheritance relations even if not documented,
        //        we do make them artificial, so they do not appear in the index
        //if (!Config_getBool(HIDE_UNDOC_RELATIONS))
        bool b = Config_getBool(HIDE_UNDOC_RELATIONS) ? TRUE : isArtificial;
        //{
          // no documented base class -> try to find an undocumented one
          findClassRelation(root,context,instanceCd,&tbi,templateNames,Undocumented,b);
        //}
      }
    }
    else if (mode==TemplateInstances)
    {
      findClassRelation(root,context,instanceCd,&tbi,templateNames,TemplateInstances,isArtificial);
    }
    if (delTempNames)
    {
      delete templateNames;
      templateNames=0;
    }
  }
}

//----------------------------------------------------------------------

static bool findTemplateInstanceRelation(const Entry *root,
            Definition *context,
            ClassDef *templateClass,const QCString &templSpec,
            QDict<int> *templateNames,
            bool isArtificial)
{
  Debug::print(Debug::Classes,0,"    derived from template %s with parameters %s\n",
         qPrint(templateClass->name()),qPrint(templSpec));
  //printf("findTemplateInstanceRelation(base=%s templSpec=%s templateNames=",
  //    templateClass->name().data(),templSpec.data());
  //if (templateNames)
  //{
  //  QDictIterator<int> qdi(*templateNames);
  //  int *tempArgIndex;
  //  for (;(tempArgIndex=qdi.current());++qdi)
  //  {
  //    printf("(%s->%d) ",qdi.currentKey(),*tempArgIndex);
  //  }
  //}
  //printf("\n");

  bool existingClass = (templSpec ==
                        tempArgListToString(templateClass->templateArguments(),root->lang,false)
                       );
  if (existingClass) return TRUE;

  bool freshInstance=FALSE;
  ClassDef *instanceClass = templateClass->insertTemplateInstance(
                     root->fileName,root->startLine,root->startColumn,templSpec,freshInstance);
  if (isArtificial) instanceClass->setArtificial(TRUE);
  instanceClass->setLanguage(root->lang);

  if (freshInstance)
  {
    Debug::print(Debug::Classes,0,"      found fresh instance '%s'!\n",qPrint(instanceClass->name()));
    Doxygen::classSDict->append(instanceClass->name(),instanceClass);
    instanceClass->setTemplateBaseClassNames(templateNames);

    // search for new template instances caused by base classes of
    // instanceClass
    auto it = g_classEntries.find(templateClass->name().data());
    if (it!=g_classEntries.end())
    {
      const Entry *templateRoot = it->second;
      Debug::print(Debug::Classes,0,"        template root found %s templSpec=%s!\n",
          qPrint(templateRoot->name),qPrint(templSpec));
      std::unique_ptr<ArgumentList> templArgs = stringToArgumentList(root->lang,templSpec);
      findBaseClassesForClass(templateRoot,context,templateClass,instanceClass,
          TemplateInstances,isArtificial,templArgs,templateNames);

      findUsedClassesForClass(templateRoot,context,templateClass,instanceClass,
          isArtificial,templArgs,templateNames);
    }
    else
    {
      Debug::print(Debug::Classes,0,"        no template root entry found!\n");
      // TODO: what happened if we get here?
    }

    //Debug::print(Debug::Classes,0,"    Template instance %s : \n",instanceClass->name().data());
    //ArgumentList *tl = templateClass->templateArguments();
  }
  else
  {
    Debug::print(Debug::Classes,0,"      instance already exists!\n");
  }
  return TRUE;
}

static bool isRecursiveBaseClass(const QCString &scope,const QCString &name)
{
  QCString n=name;
  int index=n.find('<');
  if (index!=-1)
  {
    n=n.left(index);
  }
  bool result = rightScopeMatch(scope,n);
  return result;
}

/*! Searches for the end of a template in prototype \a s starting from
 *  character position \a startPos. If the end was found the position
 *  of the closing \> is returned, otherwise -1 is returned.
 *
 *  Handles exotic cases such as
 *  \code
 *    Class<(id<0)>
 *    Class<bits<<2>
 *    Class<"<">
 *    Class<'<'>
 *    Class<(")<")>
 *  \endcode
 */
static int findEndOfTemplate(const QCString &s,int startPos)
{
  // locate end of template
  int e=startPos;
  int brCount=1;
  int roundCount=0;
  int len = s.length();
  bool insideString=FALSE;
  bool insideChar=FALSE;
  char pc = 0;
  while (e<len && brCount!=0)
  {
    char c=s.at(e);
    switch(c)
    {
      case '<':
        if (!insideString && !insideChar)
        {
          if (e<len-1 && s.at(e+1)=='<')
            e++;
          else if (roundCount==0)
            brCount++;
        }
        break;
      case '>':
        if (!insideString && !insideChar)
        {
          if (e<len-1 && s.at(e+1)=='>')
            e++;
          else if (roundCount==0)
            brCount--;
        }
        break;
      case '(':
        if (!insideString && !insideChar)
          roundCount++;
        break;
      case ')':
        if (!insideString && !insideChar)
          roundCount--;
        break;
      case '"':
        if (!insideChar)
        {
          if (insideString && pc!='\\')
            insideString=FALSE;
          else
            insideString=TRUE;
        }
        break;
      case '\'':
        if (!insideString)
        {
          if (insideChar && pc!='\\')
            insideChar=FALSE;
          else
            insideChar=TRUE;
        }
        break;
    }
    pc = c;
    e++;
  }
  return brCount==0 ? e : -1;
}

static bool findClassRelation(
                           const Entry *root,
                           Definition *context,
                           ClassDef *cd,
                           const BaseInfo *bi,
                           QDict<int> *templateNames,
                           FindBaseClassRelation_Mode mode,
                           bool isArtificial
                          )
{
  //printf("findClassRelation(class=%s base=%s templateNames=",
  //    cd->name().data(),bi->name.data());
  //if (templateNames)
  //{
  //  QDictIterator<int> qdi(*templateNames);
  //  int *tempArgIndex;
  //  for (;(tempArgIndex=qdi.current());++qdi)
  //  {
  //    printf("(%s->%d) ",qdi.currentKey(),*tempArgIndex);
  //  }
  //}
  //printf("\n");

  QCString biName=bi->name;
  bool explicitGlobalScope=FALSE;
  //printf("findClassRelation: biName='%s'\n",biName.data());
  if (biName.left(2)=="::") // explicit global scope
  {
     biName=biName.right(biName.length()-2);
     explicitGlobalScope=TRUE;
  }

  Entry *parentNode=root->parent();
  bool lastParent=FALSE;
  do // for each parent scope, starting with the largest scope
     // (in case of nested classes)
  {
    QCString scopeName= parentNode ? parentNode->name.data() : "";
    int scopeOffset=explicitGlobalScope ? 0 : scopeName.length();
    do // try all parent scope prefixes, starting with the largest scope
    {
      //printf("scopePrefix='%s' biName='%s'\n",
      //    scopeName.left(scopeOffset).data(),biName.data());

      QCString baseClassName=biName;
      if (scopeOffset>0)
      {
        baseClassName.prepend(scopeName.left(scopeOffset)+"::");
      }
      //QCString stripped;
      //baseClassName=stripTemplateSpecifiersFromScope
      //                    (removeRedundantWhiteSpace(baseClassName),TRUE,
      //                    &stripped);
      const MemberDef *baseClassTypeDef=0;
      QCString templSpec;
      ClassDef *baseClass=const_cast<ClassDef*>(
                                           getResolvedClass(explicitGlobalScope ? Doxygen::globalScope : context,
                                           cd->getFileDef(),
                                           baseClassName,
                                           &baseClassTypeDef,
                                           &templSpec,
                                           mode==Undocumented,
                                           TRUE
                                          ));
      //printf("baseClassName=%s baseClass=%p cd=%p explicitGlobalScope=%d\n",
      //    baseClassName.data(),baseClass,cd,explicitGlobalScope);
      //printf("    scope='%s' baseClassName='%s' baseClass=%s templSpec=%s\n",
      //                    cd ? cd->name().data():"<none>",
      //                    baseClassName.data(),
      //                    baseClass?baseClass->name().data():"<none>",
      //                    templSpec.data()
      //      );
      //if (baseClassName.left(root->name.length())!=root->name ||
      //    baseClassName.at(root->name.length())!='<'
      //   ) // Check for base class with the same name.
      //     // If found then look in the outer scope for a match
      //     // and prevent recursion.
      if (!isRecursiveBaseClass(root->name,baseClassName)
          || explicitGlobalScope
          // sadly isRecursiveBaseClass always true for UNO IDL ifc/svc members
          // (i.e. this is needed for addInterfaceOrServiceToServiceOrSingleton)
          || (root->lang==SrcLangExt_IDL &&
              (root->section==Entry::EXPORTED_INTERFACE_SEC ||
               root->section==Entry::INCLUDED_SERVICE_SEC)))
      {
        Debug::print(
            Debug::Classes,0,"    class relation %s inherited/used by %s found (%s and %s) templSpec='%s'\n",
            qPrint(baseClassName),
            qPrint(root->name),
            (bi->prot==Private)?"private":((bi->prot==Protected)?"protected":"public"),
            (bi->virt==Normal)?"normal":"virtual",
            qPrint(templSpec)
           );

        int i=baseClassName.find('<');
        int si=baseClassName.findRev("::",i==-1 ? baseClassName.length() : i);
        if (si==-1) si=0;
        if (baseClass==0 && (root->lang==SrcLangExt_CSharp || root->lang==SrcLangExt_Java))
        {
          // for Java/C# strip the template part before looking for matching
          baseClass = Doxygen::genericsDict->find(baseClassName.left(i));
          //printf("looking for '%s' result=%p\n",baseClassName.data(),baseClass);
        }
        if (baseClass==0 && i!=-1)
          // base class has template specifiers
        {
          // TODO: here we should try to find the correct template specialization
          // but for now, we only look for the unspecialized base class.
          int e=findEndOfTemplate(baseClassName,i+1);
          //printf("baseClass==0 i=%d e=%d\n",i,e);
          if (e!=-1) // end of template was found at e
          {
            templSpec=removeRedundantWhiteSpace(baseClassName.mid(i,e-i));
            baseClassName=baseClassName.left(i)+baseClassName.right(baseClassName.length()-e);
            baseClass=const_cast<ClassDef*>(
                getResolvedClass(explicitGlobalScope ? Doxygen::globalScope : context,
                   cd->getFileDef(),
                  baseClassName,
                  &baseClassTypeDef,
                  0, //&templSpec,
                  mode==Undocumented,
                  TRUE
                  ));
            //printf("baseClass=%p -> baseClass=%s templSpec=%s\n",
            //      baseClass,baseClassName.data(),templSpec.data());
          }
        }
        else if (baseClass && !templSpec.isEmpty()) // we have a known class, but also
                                                    // know it is a template, so see if
                                                    // we can also link to the explicit
                                                    // instance (for instance if a class
                                                    // derived from a template argument)
        {
          //printf("baseClass=%s templSpec=%s\n",baseClass->name().data(),templSpec.data());
          ClassDef *templClass=getClass(baseClass->name()+templSpec);
          if (templClass)
          {
            // use the template instance instead of the template base.
            baseClass = templClass;
            templSpec.resize(0);
          }
        }

        //printf("cd=%p baseClass=%p\n",cd,baseClass);
        bool found=baseClass!=0 && (baseClass!=cd || mode==TemplateInstances);
        //printf("1. found=%d\n",found);
        if (!found && si!=-1)
        {
          QCString tmpTemplSpec;
          // replace any namespace aliases
          replaceNamespaceAliases(baseClassName,si);
          baseClass=const_cast<ClassDef*>(
                                     getResolvedClass(explicitGlobalScope ? Doxygen::globalScope : context,
                                     cd->getFileDef(),
                                     baseClassName,
                                     &baseClassTypeDef,
                                     &tmpTemplSpec,
                                     mode==Undocumented,
                                     TRUE
                                    ));
          found=baseClass!=0 && baseClass!=cd;
          if (found) templSpec = tmpTemplSpec;
        }
        //printf("2. found=%d\n",found);

        //printf("root->name=%s biName=%s baseClassName=%s\n",
        //        root->name.data(),biName.data(),baseClassName.data());
        //if (cd->isCSharp() && i!=-1) // C# generic -> add internal -g postfix
        //{
        //  baseClassName+="-g";
        //}

        if (!found)
        {
          baseClass=findClassWithinClassContext(context,cd,baseClassName);
          //printf("findClassWithinClassContext(%s,%s)=%p\n",
          //    cd->name().data(),baseClassName.data(),baseClass);
          found = baseClass!=0 && baseClass!=cd;

        }
        if (!found)
        {
          // for PHP the "use A\B as C" construct map class C to A::B, so we lookup
          // the class name also in the alias mapping.
          auto it = Doxygen::namespaceAliasMap.find(baseClassName.data());
          if (it!=Doxygen::namespaceAliasMap.end()) // see if it is indeed a class.
          {
            baseClass=getClass(it->second.c_str());
            found = baseClass!=0 && baseClass!=cd;
          }
        }
        bool isATemplateArgument = templateNames!=0 && templateNames->find(biName)!=0;
        // make templSpec canonical
        // warning: the following line doesn't work for Mixin classes (see bug 560623)
        // templSpec = getCanonicalTemplateSpec(cd, cd->getFileDef(), templSpec);

        //printf("3. found=%d\n",found);
        if (found)
        {
          Debug::print(Debug::Classes,0,"    Documented base class '%s' templSpec=%s\n",qPrint(biName),qPrint(templSpec));
          // add base class to this class

          // if templSpec is not empty then we should "instantiate"
          // the template baseClass. A new ClassDef should be created
          // to represent the instance. To be able to add the (instantiated)
          // members and documentation of a template class
          // (inserted in that template class at a later stage),
          // the template should know about its instances.
          // the instantiation process, should be done in a recursive way,
          // since instantiating a template may introduce new inheritance
          // relations.
          if (!templSpec.isEmpty() && mode==TemplateInstances)
          {
            // if baseClass is actually a typedef then we should not
            // instantiate it, since typedefs are in a different namespace
            // see bug531637 for an example where this would otherwise hang
            // doxygen
            if (baseClassTypeDef==0)
            {
              //printf("       => findTemplateInstanceRelation: %p\n",baseClassTypeDef);
              findTemplateInstanceRelation(root,context,baseClass,templSpec,templateNames,isArtificial);
            }
          }
          else if (mode==DocumentedOnly || mode==Undocumented)
          {
            //printf("       => insert base class\n");
            QCString usedName;
            if (baseClassTypeDef || cd->isCSharp())
            {
              usedName=biName;
              //printf("***** usedName=%s templSpec=%s\n",usedName.data(),templSpec.data());
            }
            Protection prot = bi->prot;
            if (Config_getBool(SIP_SUPPORT)) prot=Public;
            if (!cd->isSubClass(baseClass)) // check for recursion, see bug690787
            {
              cd->insertBaseClass(baseClass,usedName,prot,bi->virt,templSpec);
              // add this class as super class to the base class
              baseClass->insertSubClass(cd,prot,bi->virt,templSpec);
            }
            else
            {
              warn(root->fileName,root->startLine,
                  "Detected potential recursive class relation "
                  "between class %s and base class %s!",
                  cd->name().data(),baseClass->name().data()
                  );
            }
          }
          return TRUE;
        }
        else if (mode==Undocumented && (scopeOffset==0 || isATemplateArgument))
        {
          Debug::print(Debug::Classes,0,
                       "    New undocumented base class '%s' baseClassName=%s templSpec=%s isArtificial=%d\n",
                       qPrint(biName),qPrint(baseClassName),qPrint(templSpec),isArtificial
                      );
          baseClass=0;
          if (isATemplateArgument)
          {
            baseClass=Doxygen::hiddenClasses->find(baseClassName);
            if (baseClass==0)
            {
              baseClass=createClassDef(root->fileName,root->startLine,root->startColumn,
                                 baseClassName,
                                 ClassDef::Class);
              Doxygen::hiddenClasses->append(baseClassName,baseClass);
              if (isArtificial) baseClass->setArtificial(TRUE);
              baseClass->setLanguage(root->lang);
            }
          }
          else
          {
            baseClass=Doxygen::classSDict->find(baseClassName);
            //printf("*** classDDict->find(%s)=%p biName=%s templSpec=%s\n",
            //    baseClassName.data(),baseClass,biName.data(),templSpec.data());
            if (baseClass==0)
            {
              baseClass=createClassDef(root->fileName,root->startLine,root->startColumn,
                  baseClassName,
                  ClassDef::Class);
              Doxygen::classSDict->append(baseClassName,baseClass);
              if (isArtificial) baseClass->setArtificial(TRUE);
              baseClass->setLanguage(root->lang);
              si = baseClassName.findRev("::");
              if (si!=-1) // class is nested
              {
                Definition *sd = findScopeFromQualifiedName(Doxygen::globalScope,baseClassName.left(si),0,root->tagInfo());
                if (sd==0 || sd==Doxygen::globalScope) // outer scope not found
                {
                  baseClass->setArtificial(TRUE); // see bug678139
                }
              }
            }
          }
          if (biName.right(2)=="-p")
          {
            biName="<"+biName.left(biName.length()-2)+">";
          }
          // add base class to this class
          cd->insertBaseClass(baseClass,biName,bi->prot,bi->virt,templSpec);
          // add this class as super class to the base class
          baseClass->insertSubClass(cd,bi->prot,bi->virt,templSpec);
          // the undocumented base was found in this file
          baseClass->insertUsedFile(root->fileDef());
          baseClass->setOuterScope(Doxygen::globalScope);
          if (baseClassName.right(2)=="-p")
          {
            baseClass->setCompoundType(ClassDef::Protocol);
          }
          return TRUE;
        }
        else
        {
          Debug::print(Debug::Classes,0,"    Base class '%s' not found\n",qPrint(biName));
        }
      }
      else
      {
        if (mode!=TemplateInstances)
        {
          warn(root->fileName,root->startLine,
              "Detected potential recursive class relation "
              "between class %s and base class %s!\n",
              root->name.data(),baseClassName.data()
              );
        }
        // for mode==TemplateInstance this case is quite common and
        // indicates a relation between a template class and a template
        // instance with the same name.
      }
      if (scopeOffset==0)
      {
        scopeOffset=-1;
      }
      else if ((scopeOffset=scopeName.findRev("::",scopeOffset-1))==-1)
      {
        scopeOffset=0;
      }
      //printf("new scopeOffset='%d'",scopeOffset);
    } while (scopeOffset>=0);

    if (parentNode==0)
    {
      lastParent=TRUE;
    }
    else
    {
      parentNode=parentNode->parent();
    }
  } while (lastParent);

  return FALSE;
}

//----------------------------------------------------------------------
// Computes the base and super classes for each class in the tree

static bool isClassSection(const Entry *root)
{
  if ( !root->name.isEmpty() )
  {
    if (root->section & Entry::COMPOUND_MASK)
         // is it a compound (class, struct, union, interface ...)
    {
      return TRUE;
    }
    else if (root->section & Entry::COMPOUNDDOC_MASK)
         // is it a documentation block with inheritance info.
    {
      bool hasExtends = !root->extends.empty();
      if (hasExtends) return TRUE;
    }
  }
  return FALSE;
}


/*! Builds a dictionary of all entry nodes in the tree starting with \a root
 */
static void findClassEntries(const Entry *root)
{
  if (isClassSection(root))
  {
    g_classEntries.insert({root->name.data(),root});
  }
  for (const auto &e : root->children()) findClassEntries(e.get());
}

static QCString extractClassName(const Entry *root)
{
  // strip any anonymous scopes first
  QCString bName=stripAnonymousNamespaceScope(root->name);
  bName=stripTemplateSpecifiersFromScope(bName);
  int i;
  if ((root->lang==SrcLangExt_CSharp || root->lang==SrcLangExt_Java) &&
      (i=bName.find('<'))!=-1)
  {
    // a Java/C# generic class looks like a C++ specialization, so we need to strip the
    // template part before looking for matches
    bName=bName.left(i);
  }
  return bName;
}

/*! Using the dictionary build by findClassEntries(), this
 *  function will look for additional template specialization that
 *  exists as inheritance relations only. These instances will be
 *  added to the template they are derived from.
 */
static void findInheritedTemplateInstances()
{
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  for (cli.toFirst();cli.current();++cli) cli.current()->setVisited(FALSE);
  for (const auto &kv : g_classEntries)
  {
    const Entry *root = kv.second;
    ClassDef *cd;
    QCString bName = extractClassName(root);
    Debug::print(Debug::Classes,0,"  Inheritance: Class %s : \n",qPrint(bName));
    if ((cd=getClass(bName)))
    {
      //printf("Class %s %d\n",cd->name().data(),root->extends->count());
      findBaseClassesForClass(root,cd,cd,cd,TemplateInstances,FALSE);
    }
  }
}

static void findUsedTemplateInstances()
{
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  for (cli.toFirst();cli.current();++cli) cli.current()->setVisited(FALSE);
  for (const auto &kv : g_classEntries)
  {
    const Entry *root = kv.second;
    ClassDef *cd;
    QCString bName = extractClassName(root);
    Debug::print(Debug::Classes,0,"  Usage: Class %s : \n",qPrint(bName));
    if ((cd=getClass(bName)))
    {
      findUsedClassesForClass(root,cd,cd,cd,TRUE);
      cd->addTypeConstraints();
    }
  }
}

static void computeClassRelations()
{
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  for (cli.toFirst();cli.current();++cli) cli.current()->setVisited(FALSE);
  for (const auto &kv : g_classEntries)
  {
    const Entry *root = kv.second;
    ClassDef *cd;

    QCString bName = extractClassName(root);
    Debug::print(Debug::Classes,0,"  Relations: Class %s : \n",qPrint(bName));
    if ((cd=getClass(bName)))
    {
      findBaseClassesForClass(root,cd,cd,cd,DocumentedOnly,FALSE);
    }
    size_t numMembers = cd ? cd->memberNameInfoLinkedMap().size() : 0;
    if ((cd==0 || (!cd->hasDocumentation() && !cd->isReference())) && numMembers>0 &&
        bName.right(2)!="::")
    {
      if (!root->name.isEmpty() && root->name.find('@')==-1 && // normal name
          (guessSection(root->fileName)==Entry::HEADER_SEC ||
           Config_getBool(EXTRACT_LOCAL_CLASSES)) && // not defined in source file
           protectionLevelVisible(root->protection) && // hidden by protection
           !Config_getBool(HIDE_UNDOC_CLASSES) // undocumented class are visible
         )
        warn_undoc(
                   root->fileName,root->startLine,
                   "Compound %s is not documented.",
                   root->name.data()
             );
    }
  }
}

static void computeTemplateClassRelations()
{
  for (const auto &kv : g_classEntries)
  {
    const Entry *root = kv.second;
    QCString bName=stripAnonymousNamespaceScope(root->name);
    bName=stripTemplateSpecifiersFromScope(bName);
    ClassDef *cd=getClass(bName);
    // strip any anonymous scopes first
    QDict<ClassDef> *templInstances = 0;
    if (cd && (templInstances=cd->getTemplateInstances()))
    {
      Debug::print(Debug::Classes,0,"  Template class %s : \n",qPrint(cd->name()));
      QDictIterator<ClassDef> tdi(*templInstances);
      ClassDef *tcd;
      for (tdi.toFirst();(tcd=tdi.current());++tdi) // for each template instance
      {
        Debug::print(Debug::Classes,0,"    Template instance %s : \n",qPrint(tcd->name()));
        QCString templSpec = tdi.currentKey();
        std::unique_ptr<ArgumentList> templArgs = stringToArgumentList(tcd->getLanguage(),templSpec);
        for (const BaseInfo &bi : root->extends)
        {
          // check if the base class is a template argument
          BaseInfo tbi = bi;
          const ArgumentList &tl = cd->templateArguments();
          if (!tl.empty())
          {
            QDict<int> *baseClassNames = tcd->getTemplateBaseClassNames();
            QDict<int> *templateNames = getTemplateArgumentsInName(tl,bi.name);
            // for each template name that we inherit from we need to
            // substitute the formal with the actual arguments
            QDict<int> *actualTemplateNames = new QDict<int>(17);
            actualTemplateNames->setAutoDelete(TRUE);
            QDictIterator<int> qdi(*templateNames);
            for (qdi.toFirst();qdi.current();++qdi)
            {
              int templIndex = *qdi.current();
              Argument actArg;
              bool hasActArg=FALSE;
              if (templIndex<(int)templArgs->size())
              {
                actArg=templArgs->at(templIndex);
                hasActArg=TRUE;
              }
              if (hasActArg &&
                  baseClassNames!=0 &&
                  baseClassNames->find(actArg.type)!=0 &&
                  actualTemplateNames->find(actArg.type)==0
                 )
              {
                actualTemplateNames->insert(actArg.type,new int(templIndex));
              }
            }
            delete templateNames;

            tbi.name = substituteTemplateArgumentsInString(bi.name,tl,templArgs);
            // find a documented base class in the correct scope
            if (!findClassRelation(root,cd,tcd,&tbi,actualTemplateNames,DocumentedOnly,FALSE))
            {
              // no documented base class -> try to find an undocumented one
              findClassRelation(root,cd,tcd,&tbi,actualTemplateNames,Undocumented,TRUE);
            }
            delete actualTemplateNames;
          }
        }
      } // class has no base classes
    }
  }
}

//-----------------------------------------------------------------------
// compute the references (anchors in HTML) for each function in the file

static void computeMemberReferences()
{
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  ClassDef *cd=0;
  for (cli.toFirst();(cd=cli.current());++cli)
  {
    cd->computeAnchors();
  }
  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    for (const auto &fd : *fn)
    {
      fd->computeAnchors();
    }
  }
  NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
  NamespaceDef *nd=0;
  for (nli.toFirst();(nd=nli.current());++nli)
  {
    nd->computeAnchors();
  }
  GroupSDict::Iterator gli(*Doxygen::groupSDict);
  GroupDef *gd;
  for (gli.toFirst();(gd=gli.current());++gli)
  {
    gd->computeAnchors();
  }
}

//----------------------------------------------------------------------

static void addListReferences()
{
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  ClassDef *cd=0;
  for (cli.toFirst();(cd=cli.current());++cli)
  {
    if (!cd->isAlias())
    {
      cd->addListReferences();
    }
  }

  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    for (const auto &fd : *fn)
    {
      fd->addListReferences();
    }
  }

  NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
  NamespaceDef *nd=0;
  for (nli.toFirst();(nd=nli.current());++nli)
  {
    if (!nd->isAlias())
    {
      nd->addListReferences();
    }
  }

  GroupSDict::Iterator gli(*Doxygen::groupSDict);
  GroupDef *gd;
  for (gli.toFirst();(gd=gli.current());++gli)
  {
    gd->addListReferences();
  }

  PageSDict::Iterator pdi(*Doxygen::pageSDict);
  PageDef *pd=0;
  for (pdi.toFirst();(pd=pdi.current());++pdi)
  {
    QCString name = pd->getOutputFileBase();
    if (pd->getGroupDef())
    {
      name = pd->getGroupDef()->getOutputFileBase();
    }
    {
      const RefItemVector &xrefItems = pd->xrefListItems();
      addRefItem(xrefItems,
          name,
          theTranslator->trPage(TRUE,TRUE),
          name,pd->title(),0,0);
    }
  }

  DirSDict::Iterator ddi(*Doxygen::directories);
  DirDef *dd = 0;
  for (ddi.toFirst();(dd=ddi.current());++ddi)
  {
    QCString name = dd->getOutputFileBase();
    //if (dd->getGroupDef())
    //{
    //  name = dd->getGroupDef()->getOutputFileBase();
    //}
    const RefItemVector &xrefItems = dd->xrefListItems();
    addRefItem(xrefItems,
        name,
        theTranslator->trDir(TRUE,TRUE),
        name,dd->displayName(),0,0);
  }
}

//----------------------------------------------------------------------

static void generateXRefPages()
{
  for (RefListManager::Ptr &rl : RefListManager::instance())
  {
    rl->generatePage();
  }
}

//----------------------------------------------------------------------
// Copy the documentation in entry 'root' to member definition 'md' and
// set the function declaration of the member to 'funcDecl'. If the boolean
// over_load is set the standard overload text is added.

static void addMemberDocs(const Entry *root,
                   MemberDef *md, const char *funcDecl,
                   const ArgumentList *al,
                   bool over_load,
                   uint64 spec
                  )
{
  //printf("addMemberDocs: '%s'::'%s' '%s' funcDecl='%s' mSpec=%lld\n",
  //     root->parent()->name.data(),md->name().data(),md->argsString(),funcDecl,spec);
  QCString fDecl=funcDecl;
  // strip extern specifier
  fDecl.stripPrefix("extern ");
  md->setDefinition(fDecl);
  md->enableCallGraph(root->callGraph);
  md->enableCallerGraph(root->callerGraph);
  md->enableReferencedByRelation(root->referencedByRelation);
  md->enableReferencesRelation(root->referencesRelation);
  ClassDef *cd=md->getClassDef();
  const NamespaceDef *nd=md->getNamespaceDef();
  QCString fullName;
  if (cd)
    fullName = cd->name();
  else if (nd)
    fullName = nd->name();

  if (!fullName.isEmpty()) fullName+="::";
  fullName+=md->name();
  FileDef *rfd=root->fileDef();

  // TODO determine scope based on root not md
  Definition *rscope = md->getOuterScope();

  ArgumentList &mdAl = md->argumentList();
  if (al)
  {
    ArgumentList mergedAl = *al;
    //printf("merging arguments (1) docs=%d\n",root->doc.isEmpty());
    mergeArguments(mdAl,mergedAl,!root->doc.isEmpty());
  }
  else
  {
    if (
          matchArguments2( md->getOuterScope(), md->getFileDef(), &mdAl,
                           rscope,rfd,&root->argList,
                           TRUE
                         )
       )
    {
      //printf("merging arguments (2)\n");
      ArgumentList mergedArgList = root->argList;
      mergeArguments(mdAl,mergedArgList,!root->doc.isEmpty());
    }
  }
  if (over_load)  // the \overload keyword was used
  {
    QCString doc=getOverloadDocs();
    if (!root->doc.isEmpty())
    {
      doc+="<p>";
      doc+=root->doc;
    }
    md->setDocumentation(doc,root->docFile,root->docLine);
    md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
    md->setDocsForDefinition(!root->proto);
  }
  else
  {
    //printf("overwrite!\n");
    md->setDocumentation(root->doc,root->docFile,root->docLine);
    md->setDocsForDefinition(!root->proto);

    //printf("overwrite!\n");
    md->setBriefDescription(root->brief,root->briefFile,root->briefLine);

    if (
        (md->inbodyDocumentation().isEmpty() ||
         !root->parent()->name.isEmpty()
        ) && !root->inbodyDocs.isEmpty()
       )
    {
      md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
    }
  }

  //printf("initializer: '%s'(isEmpty=%d) '%s'(isEmpty=%d)\n",
  //    md->initializer().data(),md->initializer().isEmpty(),
  //    root->initializer.data(),root->initializer.isEmpty()
  //   );
  if (md->initializer().isEmpty() && !root->initializer.isEmpty())
  {
    //printf("setInitializer\n");
    md->setInitializer(root->initializer);
  }

  md->setMaxInitLines(root->initLines);

  if (rfd)
  {
    if ((md->getStartBodyLine()==-1 && root->bodyLine!=-1)
       )
    {
      //printf("Setting new body segment [%d,%d]\n",root->bodyLine,root->endBodyLine);
      md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
      md->setBodyDef(rfd);
    }

    md->setRefItems(root->sli);
  }

  md->enableCallGraph(md->hasCallGraph() || root->callGraph);
  md->enableCallerGraph(md->hasCallerGraph() || root->callerGraph);
  md->enableReferencedByRelation(md->hasReferencedByRelation() || root->referencedByRelation);
  md->enableReferencesRelation(md->hasReferencesRelation() || root->referencesRelation);

  md->mergeMemberSpecifiers(spec);
  md->addSectionsToDefinition(root->anchors);
  addMemberToGroups(root,md);
  if (cd) cd->insertUsedFile(rfd);
  //printf("root->mGrpId=%d\n",root->mGrpId);
  if (root->mGrpId!=-1)
  {
    if (md->getMemberGroupId()!=-1)
    {
      if (md->getMemberGroupId()!=root->mGrpId)
      {
        warn(
             root->fileName,root->startLine,
             "member %s belongs to two different groups. The second "
             "one found here will be ignored.",
             md->name().data()
            );
      }
    }
    else // set group id
    {
      //printf("setMemberGroupId=%d md=%s\n",root->mGrpId,md->name().data());
      md->setMemberGroupId(root->mGrpId);
    }
  }
}

//----------------------------------------------------------------------
// find a class definition given the scope name and (optionally) a
// template list specifier

static const ClassDef *findClassDefinition(FileDef *fd,NamespaceDef *nd,
                         const char *scopeName)
{
  const ClassDef *tcd = getResolvedClass(nd,fd,scopeName,0,0,TRUE,TRUE);
  return tcd;
}


//----------------------------------------------------------------------
// Adds the documentation contained in 'root' to a global function
// with name 'name' and argument list 'args' (for overloading) and
// function declaration 'decl' to the corresponding member definition.

static bool findGlobalMember(const Entry *root,
                           const QCString &namespaceName,
                           const char *type,
                           const char *name,
                           const char *tempArg,
                           const char *,
                           const char *decl,
                           uint64 spec)
{
  Debug::print(Debug::FindMembers,0,
       "2. findGlobalMember(namespace=%s,type=%s,name=%s,tempArg=%s,decl=%s)\n",
          qPrint(namespaceName),qPrint(type),qPrint(name),qPrint(tempArg),qPrint(decl));
  QCString n=name;
  if (n.isEmpty()) return FALSE;
  if (n.find("::")!=-1) return FALSE; // skip undefined class members
  MemberName *mn=Doxygen::functionNameLinkedMap->find(n+tempArg); // look in function dictionary
  if (mn==0)
  {
    mn=Doxygen::functionNameLinkedMap->find(n); // try without template arguments
  }
  if (mn) // function name defined
  {
    Debug::print(Debug::FindMembers,0,"3. Found symbol scope\n");
    //int count=0;
    bool found=FALSE;
    for (const auto &md : *mn)
    {
      const NamespaceDef *nd=0;
      if (md->isAlias() && md->getOuterScope() &&
          md->getOuterScope()->definitionType()==Definition::TypeNamespace)
      {
        nd = dynamic_cast<const NamespaceDef *>(md->getOuterScope());
      }
      else
      {
        nd = md->getNamespaceDef();
      }
      //const Definition *scope=md->getOuterScope();
      //md = md->resolveAlias();

      const FileDef *fd=root->fileDef();
      //printf("File %s\n",fd ? fd->name().data() : "<none>");
      NamespaceSDict *nl = fd ? fd->getUsedNamespaces() : 0;
      //SDict<Definition> *cl = fd ? fd->getUsedClasses()    : 0;
      //printf("NamespaceList %p\n",nl);

      // search in the list of namespaces that are imported via a
      // using declaration
      bool viaUsingDirective = nl && nd && nl->find(nd->qualifiedName())!=0;

      if ((namespaceName.isEmpty() && nd==0) ||  // not in a namespace
          (nd && nd->name()==namespaceName) ||   // or in the same namespace
          viaUsingDirective                      // member in 'using' namespace
         )
      {
        Debug::print(Debug::FindMembers,0,"4. Try to add member '%s' to scope '%s'\n",
            qPrint(md->name()),qPrint(namespaceName));

        NamespaceDef *rnd = 0;
        if (!namespaceName.isEmpty()) rnd = Doxygen::namespaceSDict->find(namespaceName);

        const ArgumentList &mdAl = const_cast<const MemberDef *>(md.get())->argumentList();
        bool matching=
          (mdAl.empty() && root->argList.empty()) ||
          md->isVariable() || md->isTypedef() || /* in case of function pointers */
          matchArguments2(md->getOuterScope(),const_cast<const MemberDef *>(md.get())->getFileDef(),&mdAl,
                          rnd ? rnd : Doxygen::globalScope,fd,&root->argList,
                          FALSE);

        // for template members we need to check if the number of
        // template arguments is the same, otherwise we are dealing with
        // different functions.
        if (matching && !root->tArgLists.empty())
        {
          const ArgumentList &mdTempl = md->templateArguments();
          if (root->tArgLists.back().size()!=mdTempl.size())
          {
            matching=FALSE;
          }
        }

        //printf("%s<->%s\n",
        //    argListToString(md->argumentList()).data(),
        //    argListToString(root->argList).data());

        // for static members we also check if the comment block was found in
        // the same file. This is needed because static members with the same
        // name can be in different files. Thus it would be wrong to just
        // put the comment block at the first syntactically matching member.
        if (matching && md->isStatic() &&
            md->getDefFileName()!=root->fileName &&
            mn->size()>1)
        {
          matching = FALSE;
        }

        // for template member we also need to check the return type
        if (!md->templateArguments().empty() && !root->tArgLists.empty())
        {
          //printf("Comparing return types '%s'<->'%s'\n",
          //    md->typeString(),type);
          if (md->templateArguments().size()!=root->tArgLists.back().size() ||
              qstrcmp(md->typeString(),type)!=0)
          {
            //printf(" ---> no matching\n");
            matching = FALSE;
          }
        }

        if (matching) // add docs to the member
        {
          Debug::print(Debug::FindMembers,0,"5. Match found\n");
          addMemberDocs(root,md->resolveAlias(),decl,&root->argList,FALSE,root->spec);
          found=TRUE;
          break;
        }
      }
    }
    if (!found && root->relatesType != Duplicate && root->section==Entry::FUNCTION_SEC) // no match
    {
      QCString fullFuncDecl=decl;
      if (!root->argList.empty()) fullFuncDecl+=argListToString(root->argList,TRUE);
      QCString warnMsg =
         QCString("no matching file member found for \n")+substitute(fullFuncDecl,"%","%%");
      if (mn->size()>0)
      {
        warnMsg+="\nPossible candidates:\n";
        for (const auto &md : *mn)
        {
          warnMsg+=" '";
          warnMsg+=substitute(md->declaration(),"%","%%");
          warnMsg+="' at line "+QCString().setNum(md->getDefLine())+
                   " of file "+md->getDefFileName()+"\n";
        }
      }
      warn(root->fileName,root->startLine, "%s", warnMsg.data());
    }
  }
  else // got docs for an undefined member!
  {
    if (root->type!="friend class" &&
        root->type!="friend struct" &&
        root->type!="friend union" &&
        root->type!="friend" &&
        (!Config_getBool(TYPEDEF_HIDES_STRUCT) ||
         root->type.find("typedef ")==-1)
       )
    {
      warn(root->fileName,root->startLine,
           "documented symbol '%s' was not declared or defined.",decl
          );
    }
  }
  return TRUE;
}

static bool isSpecialization(
                  const ArgumentLists &srcTempArgLists,
                  const ArgumentLists &dstTempArgLists
    )
{
    auto srcIt = srcTempArgLists.begin();
    auto dstIt = dstTempArgLists.begin();
    while (srcIt!=srcTempArgLists.end() && dstIt!=dstTempArgLists.end())
    {
      if ((*srcIt).size()!=(*dstIt).size()) return TRUE;
      ++srcIt;
      ++dstIt;
    }
    return FALSE;
}

static bool scopeIsTemplate(const Definition *d)
{
  bool result=FALSE;
  if (d && d->definitionType()==Definition::TypeClass)
  {
    result = !(dynamic_cast<const ClassDef*>(d))->templateArguments().empty() ||
             scopeIsTemplate(d->getOuterScope());
  }
  return result;
}

static QCString substituteTemplatesInString(
    const ArgumentLists &srcTempArgLists,
    const ArgumentLists &dstTempArgLists,
    const QCString &src
    )
{
  QCString dst;
  QRegExp re( "[A-Za-z_][A-Za-z_0-9]*");
  //printf("type=%s\n",sa->type.data());
  int i,p=0,l;
  while ((i=re.match(src,p,&l))!=-1) // for each word in srcType
  {
    bool found=FALSE;
    dst+=src.mid(p,i-p);
    QCString name=src.mid(i,l);

    auto srcIt = srcTempArgLists.begin();
    auto dstIt = dstTempArgLists.begin();
    while (srcIt!=srcTempArgLists.end() && !found)
    {
      const ArgumentList *tdAli = 0;
      std::vector<Argument>::const_iterator tdaIt;
      if (dstIt!=dstTempArgLists.end())
      {
        tdAli = &(*dstIt);
        tdaIt = tdAli->begin();
        ++dstIt;
      }

      const ArgumentList &tsaLi = *srcIt;
      for (auto tsaIt = tsaLi.begin(); tsaIt!=tsaLi.end() && !found; ++tsaIt)
      {
        Argument tsa = *tsaIt;
        const Argument *tda = 0;
        if (tdAli && tdaIt!=tdAli->end())
        {
          tda = &(*tdaIt);
          ++tdaIt;
        }
        //if (tda) printf("tsa=%s|%s tda=%s|%s\n",
        //    tsa.type.data(),tsa.name.data(),
        //    tda->type.data(),tda->name.data());
        if (name==tsa.name)
        {
          if (tda && tda->name.isEmpty())
          {
            QCString tdaName = tda->name;
            QCString tdaType = tda->type;
            int vc=0;
            if (tdaType.left(6)=="class ") vc=6;
            else if (tdaType.left(9)=="typename ") vc=9;
            if (vc>0) // convert type=="class T" to type=="class" name=="T"
            {
              tdaName = tdaType.mid(vc);
            }
            if (!tdaName.isEmpty())
            {
              name=tdaName; // substitute
              found=TRUE;
            }
          }
        }
      }

      //printf("   srcList='%s' dstList='%s faList='%s'\n",
      //  argListToString(srclali.current()).data(),
      //  argListToString(dstlali.current()).data(),
      //  funcTempArgList ? argListToString(funcTempArgList).data() : "<none>");
      ++srcIt;
    }
    dst+=name;
    p=i+l;
  }
  dst+=src.right(src.length()-p);
  //printf("  substituteTemplatesInString(%s)=%s\n",
  //    src.data(),dst.data());
  return dst;
}

static void substituteTemplatesInArgList(
                  const ArgumentLists &srcTempArgLists,
                  const ArgumentLists &dstTempArgLists,
                  const ArgumentList &src,
                  ArgumentList &dst
                 )
{
  auto dstIt = dst.begin();
  for (const Argument &sa : src)
  {
    QCString dstType =  substituteTemplatesInString(srcTempArgLists,dstTempArgLists,sa.type);
    QCString dstArray = substituteTemplatesInString(srcTempArgLists,dstTempArgLists,sa.array);
    if (dstIt == dst.end())
    {
      Argument da = sa;
      da.type  = dstType;
      da.array = dstArray;
      dst.push_back(da);
      dstIt = dst.end();
    }
    else
    {
      Argument da = *dstIt;
      da.type  = dstType;
      da.array = dstArray;
      ++dstIt;
    }
  }
  dst.setConstSpecifier(src.constSpecifier());
  dst.setVolatileSpecifier(src.volatileSpecifier());
  dst.setPureSpecifier(src.pureSpecifier());
  dst.setTrailingReturnType(substituteTemplatesInString(
                             srcTempArgLists,dstTempArgLists,
                             src.trailingReturnType()));
  //printf("substituteTemplatesInArgList: replacing %s with %s\n",
  //    argListToString(src).data(),argListToString(dst).data()
  //    );
}

//-------------------------------------------------------------------------------------------

static void addLocalObjCMethod(const Entry *root,
                        const QCString &scopeName,
                        const QCString &funcType,const QCString &funcName,const QCString &funcArgs,
                        const QCString &exceptions,const QCString &funcDecl,
                        uint64 spec)
{
  //printf("scopeName='%s' className='%s'\n",scopeName.data(),className.data());
  ClassDef *cd=0;
  if (Config_getBool(EXTRACT_LOCAL_METHODS) && (cd=getClass(scopeName)))
  {
    Debug::print(Debug::FindMembers,0,"4. Local objective C method %s\n"
        "  scopeName=%s\n",qPrint(root->name),qPrint(scopeName));
    //printf("Local objective C method '%s' of class '%s' found\n",root->name.data(),cd->name().data());
    std::unique_ptr<MemberDef> md { createMemberDef(
        root->fileName,root->startLine,root->startColumn,
        funcType,funcName,funcArgs,exceptions,
        root->protection,root->virt,root->stat,Member,
        MemberType_Function,ArgumentList(),root->argList,root->metaData) };
    md->setTagInfo(root->tagInfo());
    md->setLanguage(root->lang);
    md->setId(root->id);
    md->makeImplementationDetail();
    md->setMemberClass(cd);
    md->setDefinition(funcDecl);
    md->enableCallGraph(root->callGraph);
    md->enableCallerGraph(root->callerGraph);
    md->enableReferencedByRelation(root->referencedByRelation);
    md->enableReferencesRelation(root->referencesRelation);
    md->setDocumentation(root->doc,root->docFile,root->docLine);
    md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
    md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
    md->setDocsForDefinition(!root->proto);
    md->setPrototype(root->proto,root->fileName,root->startLine,root->startColumn);
    md->addSectionsToDefinition(root->anchors);
    md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
    FileDef *fd=root->fileDef();
    md->setBodyDef(fd);
    md->setMemberSpecifiers(spec);
    md->setMemberGroupId(root->mGrpId);
    cd->insertMember(md.get());
    cd->insertUsedFile(fd);
    md->setRefItems(root->sli);

    MemberName *mn = Doxygen::memberNameLinkedMap->add(root->name);
    mn->push_back(std::move(md));
  }
  else
  {
    // local objective C method found for class without interface
  }
}

//-------------------------------------------------------------------------------------------

static void addMemberFunction(const Entry *root,
                       MemberName *mn,
                       const QCString &scopeName,
                       const QCString &namespaceName,
                       const QCString &className,
                       const QCString &funcTyp,
                       const QCString &funcName,
                       const QCString &funcArgs,
                       const QCString &funcTempList,
                       const QCString &exceptions,
                       const QCString &type,
                       const QCString &args,
                       bool isFriend,
                       uint64 spec,
                       const QCString &relates,
                       const QCString &funcDecl,
                       bool overloaded,
                       bool isFunc)
{
  QCString funcType = funcTyp;
  int count=0;
  int noMatchCount=0;
  bool memFound=FALSE;
  for (const auto &md : *mn)
  {
    ClassDef *cd=md->getClassDef();
    Debug::print(Debug::FindMembers,0,
        "3. member definition found, "
        "scope needed='%s' scope='%s' args='%s' fileName=%s\n",
        qPrint(scopeName),cd ? qPrint(cd->name()) : "<none>",
        qPrint(md->argsString()),
        qPrint(root->fileName));
    //printf("Member %s (member scopeName=%s) (this scopeName=%s) classTempList=%s\n",md->name().data(),cd->name().data(),scopeName.data(),classTempList.data());
    FileDef *fd=root->fileDef();
    NamespaceDef *nd=0;
    if (!namespaceName.isEmpty()) nd=getResolvedNamespace(namespaceName);

    //printf("scopeName %s->%s\n",scopeName.data(),
    //       stripTemplateSpecifiersFromScope(scopeName,FALSE).data());

    const ClassDef *tcd=findClassDefinition(fd,nd,scopeName);
    if (tcd==0 && cd && stripAnonymousNamespaceScope(cd->name())==scopeName)
    {
      // don't be fooled by anonymous scopes
      tcd=cd;
    }
    //printf("Looking for %s inside nd=%s result=%p (%s) cd=%p\n",
    //    scopeName.data(),nd?nd->name().data():"<none>",tcd,tcd?tcd->name().data():"",cd);

    if (cd && tcd==cd) // member's classes match
    {
      Debug::print(Debug::FindMembers,0,
          "4. class definition %s found\n",cd->name().data());

      // get the template parameter lists found at the member declaration
      ArgumentLists declTemplArgs = cd->getTemplateParameterLists();
      const ArgumentList &templAl = md->templateArguments();
      if (!templAl.empty())
      {
        declTemplArgs.push_back(templAl);
      }

      // get the template parameter lists found at the member definition
      const ArgumentLists &defTemplArgs = root->tArgLists;
      //printf("defTemplArgs=%p\n",defTemplArgs);

      // do we replace the decl argument lists with the def argument lists?
      bool substDone=FALSE;
      ArgumentList argList;

      /* substitute the occurrences of class template names in the
       * argument list before matching
       */
      const ArgumentList &mdAl = md->argumentList();
      if (declTemplArgs.size()>0 && declTemplArgs.size()==defTemplArgs.size())
      {
        /* the function definition has template arguments
         * and the class definition also has template arguments, so
         * we must substitute the template names of the class by that
         * of the function definition before matching.
         */
        substituteTemplatesInArgList(declTemplArgs,defTemplArgs,mdAl,argList);

        substDone=TRUE;
      }
      else /* no template arguments, compare argument lists directly */
      {
        argList = mdAl;
      }

      Debug::print(Debug::FindMembers,0,
          "5. matching '%s'<=>'%s' className=%s namespaceName=%s\n",
          qPrint(argListToString(argList,TRUE)),qPrint(argListToString(root->argList,TRUE)),
          qPrint(className),qPrint(namespaceName)
          );

      bool matching=
        md->isVariable() || md->isTypedef() || // needed for function pointers
        matchArguments2(
            md->getClassDef(),md->getFileDef(),&argList,
            cd,fd,&root->argList,
            TRUE);

      if (md->getLanguage()==SrcLangExt_ObjC && md->isVariable() && (root->section&Entry::FUNCTION_SEC))
      {
        matching = FALSE; // don't match methods and attributes with the same name
      }

      // for template member we also need to check the return type
      if (!md->templateArguments().empty() && !root->tArgLists.empty())
      {
        QCString memType = md->typeString();
        memType.stripPrefix("static "); // see bug700696
        funcType=substitute(stripTemplateSpecifiersFromScope(funcType,TRUE),
            className+"::",""); // see bug700693 & bug732594
        memType=substitute(stripTemplateSpecifiersFromScope(memType,TRUE),
            className+"::",""); // see bug758900
        Debug::print(Debug::FindMembers,0,
            "5b. Comparing return types '%s'<->'%s' #args %d<->%d\n",
            qPrint(md->typeString()),qPrint(funcType),
            md->templateArguments().size(),root->tArgLists.back().size());
        if (md->templateArguments().size()!=root->tArgLists.back().size() ||
            qstrcmp(memType,funcType))
        {
          //printf(" ---> no matching\n");
          matching = FALSE;
        }
      }
      bool rootIsUserDoc = (root->section&Entry::MEMBERDOC_SEC)!=0;
      bool classIsTemplate = scopeIsTemplate(md->getClassDef());
      bool mdIsTemplate    = md->templateArguments().hasParameters();
      bool classOrMdIsTemplate = mdIsTemplate || classIsTemplate;
      bool rootIsTemplate  = !root->tArgLists.empty();
      //printf("classIsTemplate=%d mdIsTemplate=%d rootIsTemplate=%d\n",classIsTemplate,mdIsTemplate,rootIsTemplate);
      if (!rootIsUserDoc && // don't check out-of-line @fn references, see bug722457
          (mdIsTemplate || rootIsTemplate) && // either md or root is a template
          ((classOrMdIsTemplate && !rootIsTemplate) || (!classOrMdIsTemplate && rootIsTemplate))
         )
      {
        // Method with template return type does not match method without return type
        // even if the parameters are the same. See also bug709052
        Debug::print(Debug::FindMembers,0,
            "5b. Comparing return types: template v.s. non-template\n");
        matching = FALSE;
      }


      Debug::print(Debug::FindMembers,0,
          "6. match results of matchArguments2 = %d substDone=%d\n",matching,substDone);

      if (substDone) // found a new argument list
      {
        if (matching) // replace member's argument list
        {
          md->setDefinitionTemplateParameterLists(root->tArgLists);
          md->moveArgumentList(std::make_unique<ArgumentList>(argList));
        }
        else // no match
        {
          if (!funcTempList.isEmpty() &&
              isSpecialization(declTemplArgs,defTemplArgs))
          {
            // check if we are dealing with a partial template
            // specialization. In this case we add it to the class
            // even though the member arguments do not match.

            addMethodToClass(root,cd,type,md->name(),args,isFriend,
                md->protection(),md->isStatic(),md->virtualness(),spec,relates);
            return;
          }
        }
      }
      if (matching)
      {
        addMemberDocs(root,md.get(),funcDecl,0,overloaded,spec);
        count++;
        memFound=TRUE;
      }
    }
    else if (cd && cd!=tcd) // we did find a class with the same name as cd
      // but in a different namespace
    {
      noMatchCount++;
    }

    if (memFound) break;
  }
  if (count==0 && root->parent() &&
      root->parent()->section==Entry::OBJCIMPL_SEC)
  {
    addLocalObjCMethod(root,scopeName,funcType,funcName,funcArgs,exceptions,funcDecl,spec);
    return;
  }
  if (count==0 && !(isFriend && funcType=="class"))
  {
    int candidates=0;
    const ClassDef *ecd = 0, *ucd = 0;
    MemberDef *emd = 0, *umd = 0;
    //printf("Assume template class\n");
    for (const auto &md : *mn)
    {
      ClassDef *ccd=md->getClassDef();
      MemberDef *cmd=md.get();
      //printf("ccd->name()==%s className=%s\n",ccd->name().data(),className.data());
      if (ccd!=0 && rightScopeMatch(ccd->name(),className))
      {
        const ArgumentList &templAl = md->templateArguments();
        if (!root->tArgLists.empty() && !templAl.empty() &&
            root->tArgLists.back().size()<=templAl.size())
        {
          Debug::print(Debug::FindMembers,0,"7. add template specialization\n");
          addMethodToClass(root,ccd,type,md->name(),args,isFriend,
              root->protection,root->stat,root->virt,spec,relates);
          return;
        }
        if (md->argsString()==argListToString(root->argList,FALSE,FALSE))
        { // exact argument list match -> remember
          ucd = ecd = ccd;
          umd = emd = cmd;
          Debug::print(Debug::FindMembers,0,
              "7. new candidate className=%s scope=%s args=%s exact match\n",
              qPrint(className),qPrint(ccd->name()),qPrint(md->argsString()));
        }
        else // arguments do not match, but member name and scope do -> remember
        {
          ucd = ccd;
          umd = cmd;
          Debug::print(Debug::FindMembers,0,
              "7. new candidate className=%s scope=%s args=%s no match\n",
              qPrint(className),qPrint(ccd->name()),qPrint(md->argsString()));
        }
        candidates++;
      }
    }
    static bool strictProtoMatching = Config_getBool(STRICT_PROTO_MATCHING);
    if (!strictProtoMatching)
    {
      if (candidates==1 && ucd && umd)
      {
        // we didn't find an actual match on argument lists, but there is only 1 member with this
        // name in the same scope, so that has to be the one.
        addMemberDocs(root,umd,funcDecl,0,overloaded,spec);
        return;
      }
      else if (candidates>1 && ecd && emd)
      {
        // we didn't find a unique match using type resolution,
        // but one of the matches has the exact same signature so
        // we take that one.
        addMemberDocs(root,emd,funcDecl,0,overloaded,spec);
        return;
      }
    }

    QCString warnMsg = "no ";
    if (noMatchCount>1) warnMsg+="uniquely ";
    warnMsg+="matching class member found for \n";

    for (const ArgumentList &al : root->tArgLists)
    {
      warnMsg+="  template ";
      warnMsg+=tempArgListToString(al,root->lang);
      warnMsg+='\n';
    }

    QCString fullFuncDecl=funcDecl.copy();
    if (isFunc) fullFuncDecl+=argListToString(root->argList,TRUE);

    warnMsg+="  ";
    warnMsg+=fullFuncDecl;
    warnMsg+='\n';

    if (candidates>0)
    {
      warnMsg+="Possible candidates:\n";
      for (const auto &md : *mn)
      {
        ClassDef *cd=md->getClassDef();
        if (cd!=0 && rightScopeMatch(cd->name(),className))
        {
          const ArgumentList &templAl = md->templateArguments();
          warnMsg+="  '";
          if (templAl.hasParameters())
          {
            warnMsg+="template ";
            warnMsg+=tempArgListToString(templAl,root->lang);
            warnMsg+='\n';
            warnMsg+="  ";
          }
          if (md->typeString())
          {
            warnMsg+=md->typeString();
            warnMsg+=' ';
          }
          QCString qScope = cd->qualifiedNameWithTemplateParameters();
          if (!qScope.isEmpty())
            warnMsg+=qScope+"::"+md->name();
          if (md->argsString())
            warnMsg+=md->argsString();
          if (noMatchCount>1)
          {
            warnMsg+="' at line "+QCString().setNum(md->getDefLine()) +
              " of file "+md->getDefFileName();
          }
          else
            warnMsg += "'";

          warnMsg+='\n';
        }
      }
    }
    warn_simple(root->fileName,root->startLine,warnMsg);
  }
}

//-------------------------------------------------------------------------------------------

static void addMemberSpecialization(const Entry *root,
                             MemberName *mn,
                             ClassDef *cd,
                             const QCString &funcType,
                             const QCString &funcName,
                             const QCString &funcArgs,
                             const QCString &funcDecl,
                             const QCString &exceptions,
                             uint64 spec
                            )
{
  MemberDef *declMd=0;
  for (const auto &md : *mn)
  {
    if (md->getClassDef()==cd)
    {
      // TODO: we should probably also check for matching arguments
      declMd = md.get();
      break;
    }
  }
  MemberType mtype=MemberType_Function;
  ArgumentList tArgList;
  //  getTemplateArgumentsFromName(cd->name()+"::"+funcName,root->tArgLists);
  std::unique_ptr<MemberDef> md { createMemberDef(
      root->fileName,root->startLine,root->startColumn,
      funcType,funcName,funcArgs,exceptions,
      declMd ? declMd->protection() : root->protection,
      root->virt,root->stat,Member,
      mtype,tArgList,root->argList,root->metaData) };
  //printf("new specialized member %s args='%s'\n",md->name().data(),funcArgs.data());
  md->setTagInfo(root->tagInfo());
  md->setLanguage(root->lang);
  md->setId(root->id);
  md->setMemberClass(cd);
  md->setTemplateSpecialization(TRUE);
  md->setTypeConstraints(root->typeConstr);
  md->setDefinition(funcDecl);
  md->enableCallGraph(root->callGraph);
  md->enableCallerGraph(root->callerGraph);
  md->enableReferencedByRelation(root->referencedByRelation);
  md->enableReferencesRelation(root->referencesRelation);
  md->setDocumentation(root->doc,root->docFile,root->docLine);
  md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
  md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
  md->setDocsForDefinition(!root->proto);
  md->setPrototype(root->proto,root->fileName,root->startLine,root->startColumn);
  md->addSectionsToDefinition(root->anchors);
  md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
  FileDef *fd=root->fileDef();
  md->setBodyDef(fd);
  md->setMemberSpecifiers(spec);
  md->setMemberGroupId(root->mGrpId);
  cd->insertMember(md.get());
  md->setRefItems(root->sli);

  mn->push_back(std::move(md));
}

//-------------------------------------------------------------------------------------------

static void addOverloaded(const Entry *root,MemberName *mn,
                          const QCString &funcType,const QCString &funcName,const QCString &funcArgs,
                          const QCString &funcDecl,const QCString &exceptions,uint64 spec)
{
  // for unique overloaded member we allow the class to be
  // omitted, this is to be Qt compatible. Using this should
  // however be avoided, because it is error prone
  bool sameClass=false;
  if (mn->size()>0)
  {
    // check if all members with the same name are also in the same class
    sameClass = std::equal(mn->begin()+1,mn->end(),mn->begin(),
                  [](const auto &md1,const auto &md2)
                  { return md1->getClassDef()->name()==md2->getClassDef()->name(); });
  }
  if (sameClass)
  {
    ClassDef *cd = mn->front()->getClassDef();
    MemberType mtype;
    if      (root->mtype==Signal)  mtype=MemberType_Signal;
    else if (root->mtype==Slot)    mtype=MemberType_Slot;
    else if (root->mtype==DCOP)    mtype=MemberType_DCOP;
    else                           mtype=MemberType_Function;

    // new overloaded member function
    std::unique_ptr<ArgumentList> tArgList =
      getTemplateArgumentsFromName(cd->name()+"::"+funcName,root->tArgLists);
    //printf("new related member %s args='%s'\n",md->name().data(),funcArgs.data());
    std::unique_ptr<MemberDef> md { createMemberDef(
        root->fileName,root->startLine,root->startColumn,
        funcType,funcName,funcArgs,exceptions,
        root->protection,root->virt,root->stat,Related,
        mtype,tArgList ? *tArgList : ArgumentList(),root->argList,root->metaData) };
    md->setTagInfo(root->tagInfo());
    md->setLanguage(root->lang);
    md->setId(root->id);
    md->setTypeConstraints(root->typeConstr);
    md->setMemberClass(cd);
    md->setDefinition(funcDecl);
    md->enableCallGraph(root->callGraph);
    md->enableCallerGraph(root->callerGraph);
    md->enableReferencedByRelation(root->referencedByRelation);
    md->enableReferencesRelation(root->referencesRelation);
    QCString doc=getOverloadDocs();
    doc+="<p>";
    doc+=root->doc;
    md->setDocumentation(doc,root->docFile,root->docLine);
    md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
    md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
    md->setDocsForDefinition(!root->proto);
    md->setPrototype(root->proto,root->fileName,root->startLine,root->startColumn);
    md->addSectionsToDefinition(root->anchors);
    md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
    FileDef *fd=root->fileDef();
    md->setBodyDef(fd);
    md->setMemberSpecifiers(spec);
    md->setMemberGroupId(root->mGrpId);
    cd->insertMember(md.get());
    cd->insertUsedFile(fd);
    md->setRefItems(root->sli);

    mn->push_back(std::move(md));
  }
}

//-------------------------------------------------------------------------------------------

/*! This function tries to find a member (in a documented class/file/namespace)
 * that corresponds to the function/variable declaration given in \a funcDecl.
 *
 * The boolean \a overloaded is used to specify whether or not a standard
 * overload documentation line should be generated.
 *
 * The boolean \a isFunc is a hint that indicates that this is a function
 * instead of a variable or typedef.
 */
static void findMember(const Entry *root,
                       const QCString &relates,
                       const QCString &type,
                       const QCString &args,
                       QCString funcDecl,
                       bool overloaded,
                       bool isFunc
                      )
{
  Debug::print(Debug::FindMembers,0,
               "findMember(root=%p,funcDecl='%s',related='%s',overload=%d,"
               "isFunc=%d mGrpId=%d #tArgList=%d "
               "spec=%lld lang=%x\n",
               root,qPrint(funcDecl),qPrint(relates),overloaded,isFunc,root->mGrpId,
               root->tArgLists.size(),
               root->spec,root->lang
              );

  QCString scopeName;
  QCString className;
  QCString namespaceName;
  QCString funcType;
  QCString funcName;
  QCString funcArgs;
  QCString funcTempList;
  QCString exceptions;
  QCString funcSpec;
  bool isRelated=FALSE;
  bool isMemberOf=FALSE;
  bool isFriend=FALSE;
  bool done;
  uint64 spec = root->spec;
  do
  {
    done=TRUE;
    if (funcDecl.stripPrefix("friend ")) // treat friends as related members
    {
      isFriend=TRUE;
      done=FALSE;
    }
    if (funcDecl.stripPrefix("inline "))
    {
      spec|=Entry::Inline;
      done=FALSE;
    }
    if (funcDecl.stripPrefix("explicit "))
    {
      spec|=Entry::Explicit;
      done=FALSE;
    }
    if (funcDecl.stripPrefix("mutable "))
    {
      spec|=Entry::Mutable;
      done=FALSE;
    }
    if (funcDecl.stripPrefix("virtual "))
    {
      done=FALSE;
    }
  } while (!done);

  // delete any ; from the function declaration
  int sep;
  while ((sep=funcDecl.find(';'))!=-1)
  {
    funcDecl=(funcDecl.left(sep)+funcDecl.right(funcDecl.length()-sep-1)).stripWhiteSpace();
  }

  // make sure the first character is a space to simplify searching.
  if (!funcDecl.isEmpty() && funcDecl[0]!=' ') funcDecl.prepend(" ");

  // remove some superfluous spaces
  funcDecl= substitute(
              substitute(
                substitute(funcDecl,"~ ","~"),
                ":: ","::"
              ),
              " ::","::"
            ).stripWhiteSpace();

  //printf("funcDecl='%s'\n",funcDecl.data());
  if (isFriend && funcDecl.left(6)=="class ")
  {
    //printf("friend class\n");
    funcDecl=funcDecl.right(funcDecl.length()-6);
    funcName = funcDecl.copy();
  }
  else if (isFriend && funcDecl.left(7)=="struct ")
  {
    funcDecl=funcDecl.right(funcDecl.length()-7);
    funcName = funcDecl.copy();
  }
  else
  {
    // extract information from the declarations
    parseFuncDecl(funcDecl,root->lang,scopeName,funcType,funcName,
                funcArgs,funcTempList,exceptions
               );
  }
  //printf("scopeName='%s' funcType='%s' funcName='%s' funcArgs='%s'\n",
  //    scopeName.data(),funcType.data(),funcName.data(),funcArgs.data());

  // the class name can also be a namespace name, we decide this later.
  // if a related class name is specified and the class name could
  // not be derived from the function declaration, then use the
  // related field.
  //printf("scopeName='%s' className='%s' namespaceName='%s'\n",
  //    scopeName.data(),className.data(),namespaceName.data());
  if (!relates.isEmpty())
  {                             // related member, prefix user specified scope
    isRelated=TRUE;
    isMemberOf=(root->relatesType == MemberOf);
    if (getClass(relates)==0 && !scopeName.isEmpty())
    {
      scopeName= mergeScopes(scopeName,relates);
    }
    else
    {
      scopeName = relates;
    }
  }

  if (relates.isEmpty() && root->parent() &&
      ((root->parent()->section&Entry::SCOPE_MASK) ||
       (root->parent()->section==Entry::OBJCIMPL_SEC)
      ) &&
      !root->parent()->name.isEmpty()) // see if we can combine scopeName
                                     // with the scope in which it was found
  {
    QCString joinedName = root->parent()->name+"::"+scopeName;
    if (!scopeName.isEmpty() &&
        (getClass(joinedName) || Doxygen::namespaceSDict->find(joinedName)))
    {
      scopeName = joinedName;
    }
    else
    {
      scopeName = mergeScopes(root->parent()->name,scopeName);
    }
  }
  else // see if we can prefix a namespace or class that is used from the file
  {
     FileDef *fd=root->fileDef();
     if (fd)
     {
       NamespaceSDict *fnl = fd->getUsedNamespaces();
       if (fnl)
       {
         QCString joinedName;
         NamespaceDef *fnd;
         NamespaceSDict::Iterator nsdi(*fnl);
         for (nsdi.toFirst();(fnd=nsdi.current());++nsdi)
         {
           joinedName = fnd->name()+"::"+scopeName;
           if (Doxygen::namespaceSDict->find(joinedName))
           {
             scopeName=joinedName;
             break;
           }
         }
       }
     }
  }
  scopeName=stripTemplateSpecifiersFromScope(
      removeRedundantWhiteSpace(scopeName),FALSE,&funcSpec);

  // funcSpec contains the last template specifiers of the given scope.
  // If this method does not have any template arguments or they are
  // empty while funcSpec is not empty we assume this is a
  // specialization of a method. If not, we clear the funcSpec and treat
  // this as a normal method of a template class.
  if (!(root->tArgLists.size()>0 &&
        root->tArgLists.front().size()==0
       )
     )
  {
    funcSpec.resize(0);
  }

  // split scope into a namespace and a class part
  extractNamespaceName(scopeName,className,namespaceName,TRUE);
  //printf("scopeName='%s' className='%s' namespaceName='%s'\n",
  //       scopeName.data(),className.data(),namespaceName.data());

  //namespaceName=removeAnonymousScopes(namespaceName);
  if (namespaceName.find('@')!=-1) return; // skip stuff in anonymous namespace...

  //printf("namespaceName='%s' className='%s'\n",namespaceName.data(),className.data());
  // merge class and namespace scopes again
  scopeName.resize(0);
  if (!namespaceName.isEmpty())
  {
    if (className.isEmpty())
    {
      scopeName=namespaceName;
    }
    else if (!relates.isEmpty() || // relates command with explicit scope
             !getClass(className)) // class name only exists in a namespace
    {
      scopeName=namespaceName+"::"+className;
    }
    else
    {
      scopeName=className;
    }
  }
  else if (!className.isEmpty())
  {
    scopeName=className;
  }
  //printf("new scope='%s'\n",scopeName.data());

  QCString tempScopeName=scopeName;
  ClassDef *cd=getClass(scopeName);
  if (cd)
  {
    if (funcSpec.isEmpty())
    {
      uint argListIndex=0;
      tempScopeName=cd->qualifiedNameWithTemplateParameters(&root->tArgLists,&argListIndex);
    }
    else
    {
      tempScopeName=scopeName+funcSpec;
    }
  }
  //printf("scopeName=%s cd=%p root->tArgLists=%p result=%s\n",
  //    scopeName.data(),cd,root->tArgLists,tempScopeName.data());

  //printf("scopeName='%s' className='%s'\n",scopeName.data(),className.data());
  // rebuild the function declaration (needed to get the scope right).
  if (!scopeName.isEmpty() && !isRelated && !isFriend && !Config_getBool(HIDE_SCOPE_NAMES))
  {
    if (!funcType.isEmpty())
    {
      if (isFunc) // a function -> we use argList for the arguments
      {
        funcDecl=funcType+" "+tempScopeName+"::"+funcName+funcTempList;
      }
      else
      {
        funcDecl=funcType+" "+tempScopeName+"::"+funcName+funcArgs;
      }
    }
    else
    {
      if (isFunc) // a function => we use argList for the arguments
      {
        funcDecl=tempScopeName+"::"+funcName+funcTempList;
      }
      else // variable => add 'argument' list
      {
        funcDecl=tempScopeName+"::"+funcName+funcArgs;
      }
    }
  }
  else // build declaration without scope
  {
    if (!funcType.isEmpty()) // but with a type
    {
      if (isFunc) // function => omit argument list
      {
        funcDecl=funcType+" "+funcName+funcTempList;
      }
      else // variable => add 'argument' list
      {
        funcDecl=funcType+" "+funcName+funcArgs;
      }
    }
    else // no type
    {
      if (isFunc)
      {
        funcDecl=funcName+funcTempList;
      }
      else
      {
        funcDecl=funcName+funcArgs;
      }
    }
  }

  if (funcType=="template class" && !funcTempList.isEmpty())
    return;   // ignore explicit template instantiations

  Debug::print(Debug::FindMembers,0,
           "findMember() Parse results:\n"
           "  namespaceName='%s'\n"
           "  className=`%s`\n"
           "  funcType='%s'\n"
           "  funcSpec='%s'\n"
           "  funcName='%s'\n"
           "  funcArgs='%s'\n"
           "  funcTempList='%s'\n"
           "  funcDecl='%s'\n"
           "  related='%s'\n"
           "  exceptions='%s'\n"
           "  isRelated=%d\n"
           "  isMemberOf=%d\n"
           "  isFriend=%d\n"
           "  isFunc=%d\n\n",
           qPrint(namespaceName),qPrint(className),
           qPrint(funcType),qPrint(funcSpec),qPrint(funcName),qPrint(funcArgs),qPrint(funcTempList),
           qPrint(funcDecl),qPrint(relates),qPrint(exceptions),isRelated,isMemberOf,isFriend,
           isFunc
          );

  MemberName *mn=0;
  if (!funcName.isEmpty()) // function name is valid
  {
    Debug::print(Debug::FindMembers,0,
                 "1. funcName='%s'\n",funcName.data());
    if (funcName.left(9)=="operator ") // strip class scope from cast operator
    {
      funcName = substitute(funcName,className+"::","");
    }
    if (!funcTempList.isEmpty()) // try with member specialization
    {
      mn=Doxygen::memberNameLinkedMap->find(funcName+funcTempList);
    }
    if (mn==0) // try without specialization
    {
      mn=Doxygen::memberNameLinkedMap->find(funcName);
    }
    if (!isRelated && mn) // function name already found
    {
      Debug::print(Debug::FindMembers,0,
                   "2. member name exists (%d members with this name)\n",mn->size());
      if (!className.isEmpty()) // class name is valid
      {
        if (funcSpec.isEmpty()) // not a member specialization
        {
          addMemberFunction(root,mn,scopeName,namespaceName,className,funcType,funcName,
                            funcArgs,funcTempList,exceptions,
                            type,args,isFriend,spec,relates,funcDecl,overloaded,isFunc);
        }
        else if (cd) // member specialization
        {
          addMemberSpecialization(root,mn,cd,funcType,funcName,funcArgs,funcDecl,exceptions,spec);
        }
        else
        {
          //printf("*** Specialized member %s of unknown scope %s%s found!\n",
          //        scopeName.data(),funcName.data(),funcArgs.data());
        }
      }
      else if (overloaded) // check if the function belongs to only one class
      {
        addOverloaded(root,mn,funcType,funcName,funcArgs,funcDecl,exceptions,spec);
      }
      else // unrelated function with the same name as a member
      {
        if (!findGlobalMember(root,namespaceName,funcType,funcName,funcTempList,funcArgs,funcDecl,spec))
        {
          QCString fullFuncDecl=funcDecl.copy();
          if (isFunc) fullFuncDecl+=argListToString(root->argList,TRUE);
          warn(root->fileName,root->startLine,
               "Cannot determine class for function\n%s",
               fullFuncDecl.data()
              );
        }
      }
    }
    else if (isRelated && !relates.isEmpty())
    {
      Debug::print(Debug::FindMembers,0,"2. related function\n"
              "  scopeName=%s className=%s\n",qPrint(scopeName),qPrint(className));
      if (className.isEmpty()) className=relates;
      //printf("scopeName='%s' className='%s'\n",scopeName.data(),className.data());
      if ((cd=getClass(scopeName)))
      {
        bool newMember=TRUE; // assume we have a new member
        MemberDef *mdDefine=0;
        {
          mn = Doxygen::functionNameLinkedMap->find(funcName);
          if (mn)
          {
            for (const auto &md : *mn)
            {
              if (md->isDefine())
              {
                mdDefine = md.get();
                break;
              }
            }
          }
        }

        FileDef *fd=root->fileDef();

        if ((mn=Doxygen::memberNameLinkedMap->find(funcName))==0)
        {
          mn=Doxygen::memberNameLinkedMap->add(funcName);
        }
        else
        {
          // see if we got another member with matching arguments
          MemberDef *rmd_found = 0;
          for (const auto &rmd : *mn)
          {
            const ArgumentList &rmdAl = rmd->argumentList();

            newMember=
              className!=rmd->getOuterScope()->name() ||
              !matchArguments2(rmd->getOuterScope(),rmd->getFileDef(),&rmdAl,
                               cd,fd,&root->argList,
                               TRUE);
            if (!newMember)
            {
              rmd_found = rmd.get();
            }
          }
          if (rmd_found) // member already exists as rmd -> add docs
          {
            //printf("addMemberDocs for related member %s\n",root->name.data());
            //rmd->setMemberDefTemplateArguments(root->mtArgList);
            addMemberDocs(root,rmd_found,funcDecl,0,overloaded,spec);
          }
        }

        if (newMember) // need to create a new member
        {
          MemberType mtype;
          if (mdDefine)
            mtype=MemberType_Define;
          else if (root->mtype==Signal)
            mtype=MemberType_Signal;
          else if (root->mtype==Slot)
            mtype=MemberType_Slot;
          else if (root->mtype==DCOP)
            mtype=MemberType_DCOP;
          else
            mtype=MemberType_Function;

          if (mdDefine)
          {
            mdDefine->setHidden(TRUE);
            funcType="#define";
            funcArgs=mdDefine->argsString();
            funcDecl=funcType + " " + funcName;
          }

          //printf("New related name '%s' '%d'\n",funcName.data(),
          //    root->argList ? (int)root->argList->count() : -1);

          // first note that we pass:
          //   (root->tArgLists ? root->tArgLists->last() : 0)
          // for the template arguments for the new "member."
          // this accurately reflects the template arguments of
          // the related function, which don't have to do with
          // those of the related class.
          std::unique_ptr<MemberDef> md { createMemberDef(
              root->fileName,root->startLine,root->startColumn,
              funcType,funcName,funcArgs,exceptions,
              root->protection,root->virt,
              root->stat && !isMemberOf,
              isMemberOf ? Foreign : Related,
              mtype,
              (!root->tArgLists.empty() ? root->tArgLists.back() : ArgumentList()),
              funcArgs.isEmpty() ? ArgumentList() : root->argList,
              root->metaData) };

          if (mdDefine)
          {
            md->setInitializer(mdDefine->initializer());
          }

          //
          // we still have the problem that
          // MemberDef::writeDocumentation() in memberdef.cpp
          // writes the template argument list for the class,
          // as if this member is a member of the class.
          // fortunately, MemberDef::writeDocumentation() has
          // a special mechanism that allows us to totally
          // override the set of template argument lists that
          // are printed.  We use that and set it to the
          // template argument lists of the related function.
          //
          md->setDefinitionTemplateParameterLists(root->tArgLists);

          md->setTagInfo(root->tagInfo());

          //printf("Related member name='%s' decl='%s' bodyLine='%d'\n",
          //       funcName.data(),funcDecl.data(),root->bodyLine);

          // try to find the matching line number of the body from the
          // global function list
          bool found=FALSE;
          if (root->bodyLine==-1)
          {
            MemberName *rmn=Doxygen::functionNameLinkedMap->find(funcName);
            if (rmn)
            {
              const MemberDef *rmd_found=0;
              for (const auto &rmd : *rmn)
              {
                const ArgumentList &rmdAl = rmd->argumentList();
                // check for matching argument lists
                if (
                    matchArguments2(rmd->getOuterScope(),rmd->getFileDef(),&rmdAl,
                                    cd,fd,&root->argList,
                                    TRUE)
                   )
                {
                  found=TRUE;
                  rmd_found = rmd.get();
                  break;
                }
              }
              if (rmd_found) // member found -> copy line number info
              {
                md->setBodySegment(rmd_found->getDefLine(),rmd_found->getStartBodyLine(),rmd_found->getEndBodyLine());
                md->setBodyDef(rmd_found->getBodyDef());
                //md->setBodyMember(rmd);
              }
            }
          }
          if (!found) // line number could not be found or is available in this
                      // entry
          {
            md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
            md->setBodyDef(fd);
          }

          //if (root->mGrpId!=-1)
          //{
          //  md->setMemberGroup(memberGroupDict[root->mGrpId]);
          //}
          md->setMemberClass(cd);
          md->setMemberSpecifiers(spec);
          md->setDefinition(funcDecl);
          md->enableCallGraph(root->callGraph);
          md->enableCallerGraph(root->callerGraph);
          md->enableReferencedByRelation(root->referencedByRelation);
          md->enableReferencesRelation(root->referencesRelation);
          md->setDocumentation(root->doc,root->docFile,root->docLine);
          md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
          md->setDocsForDefinition(!root->proto);
          md->setPrototype(root->proto,root->fileName,root->startLine,root->startColumn);
          md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
          md->addSectionsToDefinition(root->anchors);
          md->setMemberGroupId(root->mGrpId);
          md->setLanguage(root->lang);
          md->setId(root->id);
          //md->setMemberDefTemplateArguments(root->mtArgList);
          cd->insertMember(md.get());
          cd->insertUsedFile(fd);
          md->setRefItems(root->sli);
          if (root->relatesType == Duplicate) md->setRelatedAlso(cd);
          if (!mdDefine)
          {
            addMemberToGroups(root,md.get());
          }
          //printf("Adding member=%s\n",md->name().data());
          mn->push_back(std::move(md));
        }
        if (root->relatesType == Duplicate)
        {
          if (!findGlobalMember(root,namespaceName,funcType,funcName,funcTempList,funcArgs,funcDecl,spec))
          {
            QCString fullFuncDecl=funcDecl.copy();
            if (isFunc) fullFuncDecl+=argListToString(root->argList,TRUE);
            warn(root->fileName,root->startLine,
               "Cannot determine file/namespace for relatedalso function\n%s",
               fullFuncDecl.data()
              );
          }
        }
      }
      else
      {
        warn_undoc(root->fileName,root->startLine,
                   "class '%s' for related function '%s' is not "
                   "documented.",
                   className.data(),funcName.data()
                  );
      }
    }
    else if (root->parent() && root->parent()->section==Entry::OBJCIMPL_SEC)
    {
      addLocalObjCMethod(root,scopeName,funcType,funcName,funcArgs,exceptions,funcDecl,spec);
    }
    else // unrelated not overloaded member found
    {
      bool globMem = findGlobalMember(root,namespaceName,funcType,funcName,funcTempList,funcArgs,funcDecl,spec);
      if (className.isEmpty() && !globMem)
      {
        warn(root->fileName,root->startLine,
             "class for member '%s' cannot "
             "be found.", funcName.data()
            );
      }
      else if (!className.isEmpty() && !globMem)
      {
        warn(root->fileName,root->startLine,
             "member '%s' of class '%s' cannot be found",
             funcName.data(),className.data());
      }
    }
  }
  else
  {
    // this should not be called
    warn(root->fileName,root->startLine,
         "member with no name found.");
  }
  return;
}

//----------------------------------------------------------------------
// find the members corresponding to the different documentation blocks
// that are extracted from the sources.

static void filterMemberDocumentation(const Entry *root,const QCString relates)
{
  int i=-1,l;
  Debug::print(Debug::FindMembers,0,
      "findMemberDocumentation(): root->type='%s' root->inside='%s' root->name='%s' root->args='%s' section=%x root->spec=%lld root->mGrpId=%d\n",
      qPrint(root->type),qPrint(root->inside),qPrint(root->name),qPrint(root->args),root->section,root->spec,root->mGrpId
      );
  //printf("root->parent()->name=%s\n",root->parent()->name.data());
  bool isFunc=TRUE;

  QCString type = root->type;
  QCString args = root->args;
  if ( // detect func variable/typedef to func ptr
      (i=findFunctionPtr(type,root->lang,&l))!=-1
     )
  {
    //printf("Fixing function pointer!\n");
    // fix type and argument
    args.prepend(type.right(type.length()-i-l));
    type=type.left(i+l);
    //printf("Results type=%s,name=%s,args=%s\n",type.data(),root->name.data(),args.data());
    isFunc=FALSE;
  }
  else if ((type.left(8)=="typedef " && args.find('(')!=-1))
    // detect function types marked as functions
  {
    isFunc=FALSE;
  }

  //printf("Member %s isFunc=%d\n",root->name.data(),isFunc);
  if (root->section==Entry::MEMBERDOC_SEC)
  {
    //printf("Documentation for inline member '%s' found args='%s'\n",
    //    root->name.data(),args.data());
    //if (relates.length()) printf("  Relates %s\n",relates.data());
    if (type.isEmpty())
    {
      findMember(root,
                 relates,
                 type,
                 args,
                 root->name + args + root->exception,
                 FALSE,
                 isFunc);
    }
    else
    {
      findMember(root,
                 relates,
                 type,
                 args,
                 type + " " + root->name + args + root->exception,
                 FALSE,
                 isFunc);
    }
  }
  else if (root->section==Entry::OVERLOADDOC_SEC)
  {
    //printf("Overloaded member %s found\n",root->name.data());
    findMember(root,
               relates,
               type,
               args,
               root->name,
               TRUE,
               isFunc);
  }
  else if
    ((root->section==Entry::FUNCTION_SEC      // function
      ||
      (root->section==Entry::VARIABLE_SEC &&  // variable
       !type.isEmpty() &&                // with a type
       g_compoundKeywordDict.find(type)==0 // that is not a keyword
       // (to skip forward declaration of class etc.)
      )
     )
    )
    {
      //printf("Documentation for member '%s' found args='%s' excp='%s'\n",
      //    root->name.data(),args.data(),root->exception.data());
      //if (relates.length()) printf("  Relates %s\n",relates.data());
      //printf("Inside=%s\n Relates=%s\n",root->inside.data(),relates.data());
      if (type=="friend class" || type=="friend struct" ||
          type=="friend union")
      {
        findMember(root,
            relates,
            type,
            args,
            type+" "+root->name,
            FALSE,FALSE);

      }
      else if (!type.isEmpty())
      {
        findMember(root,
            relates,
            type,
            args,
            type+" "+ root->inside + root->name + args + root->exception,
            FALSE,isFunc);
      }
      else
      {
        findMember(root,
            relates,
            type,
            args,
            root->inside + root->name + args + root->exception,
            FALSE,isFunc);
      }
    }
  else if (root->section==Entry::DEFINE_SEC && !relates.isEmpty())
  {
    findMember(root,
               relates,
               type,
               args,
               root->name + args,
               FALSE,
               !args.isEmpty());
  }
  else if (root->section==Entry::VARIABLEDOC_SEC)
  {
    //printf("Documentation for variable %s found\n",root->name.data());
    //if (!relates.isEmpty()) printf("  Relates %s\n",relates.data());
    findMember(root,
               relates,
               type,
               args,
               root->name,
               FALSE,
               FALSE);
  }
  else if (root->section==Entry::EXPORTED_INTERFACE_SEC ||
           root->section==Entry::INCLUDED_SERVICE_SEC)
  {
    findMember(root,
               relates,
               type,
               args,
               type + " " + root->name,
               FALSE,
               FALSE);
  }
  else
  {
    // skip section
    //printf("skip section\n");
  }
}

static void findMemberDocumentation(const Entry *root)
{
  if (root->section==Entry::MEMBERDOC_SEC ||
      root->section==Entry::OVERLOADDOC_SEC ||
      root->section==Entry::FUNCTION_SEC ||
      root->section==Entry::VARIABLE_SEC ||
      root->section==Entry::VARIABLEDOC_SEC ||
      root->section==Entry::DEFINE_SEC ||
      root->section==Entry::INCLUDED_SERVICE_SEC ||
      root->section==Entry::EXPORTED_INTERFACE_SEC
     )
  {
    if (root->relatesType == Duplicate && !root->relates.isEmpty())
    {
      filterMemberDocumentation(root,"");
    }
    filterMemberDocumentation(root,root->relates);
  }
  for (const auto &e : root->children())
  {
    if (e->section!=Entry::ENUM_SEC)
    {
      findMemberDocumentation(e.get());
    }
  }
}

//----------------------------------------------------------------------

static void findObjCMethodDefinitions(const Entry *root)
{
  for (const auto &objCImpl : root->children())
  {
    if (objCImpl->section==Entry::OBJCIMPL_SEC)
    {
      for (const auto &objCMethod : objCImpl->children())
      {
        if (objCMethod->section==Entry::FUNCTION_SEC)
        {
          //Printf("  Found ObjC method definition %s\n",objCMethod->name.data());
          findMember(objCMethod.get(),
                     objCMethod->relates,
                     objCMethod->type,
                     objCMethod->args,
                     objCMethod->type+" "+objCImpl->name+"::"+objCMethod->name+" "+objCMethod->args,
                     FALSE,TRUE);
          objCMethod->section=Entry::EMPTY_SEC;
        }
      }
    }
  }
}

//----------------------------------------------------------------------
// find and add the enumeration to their classes, namespaces or files

static void findEnums(const Entry *root)
{
  if (root->section==Entry::ENUM_SEC)
  {
    ClassDef       *cd=0;
    FileDef        *fd=0;
    NamespaceDef   *nd=0;
    MemberNameLinkedMap *mnsd=0;
    bool isGlobal;
    bool isRelated=FALSE;
    bool isMemberOf=FALSE;
    //printf("Found enum with name '%s' relates=%s\n",root->name.data(),root->relates.data());
    int i;

    QCString name;
    QCString scope;

    if ((i=root->name.findRev("::"))!=-1) // scope is specified
    {
      scope=root->name.left(i); // extract scope
      name=root->name.right(root->name.length()-i-2); // extract name
      if ((cd=getClass(scope))==0) nd=getResolvedNamespace(scope);
    }
    else // no scope, check the scope in which the docs where found
    {
      if (( root->parent()->section & Entry::SCOPE_MASK )
          && !root->parent()->name.isEmpty()
         ) // found enum docs inside a compound
      {
        scope=root->parent()->name;
        if ((cd=getClass(scope))==0) nd=getResolvedNamespace(scope);
      }
      name=root->name;
    }

    if (!root->relates.isEmpty())
    {   // related member, prefix user specified scope
      isRelated=TRUE;
      isMemberOf=(root->relatesType == MemberOf);
      if (getClass(root->relates)==0 && !scope.isEmpty())
        scope=mergeScopes(scope,root->relates);
      else
        scope=root->relates.copy();
      if ((cd=getClass(scope))==0) nd=getResolvedNamespace(scope);
    }

    if (cd && !name.isEmpty()) // found a enum inside a compound
    {
      //printf("Enum '%s'::'%s'\n",cd->name().data(),name.data());
      fd=0;
      mnsd=Doxygen::memberNameLinkedMap;
      isGlobal=FALSE;
    }
    else if (nd) // found enum inside namespace
    {
      mnsd=Doxygen::functionNameLinkedMap;
      isGlobal=TRUE;
    }
    else // found a global enum
    {
      fd=root->fileDef();
      mnsd=Doxygen::functionNameLinkedMap;
      isGlobal=TRUE;
    }

    if (!name.isEmpty())
    {
      // new enum type
      std::unique_ptr<MemberDef> md { createMemberDef(
          root->fileName,root->startLine,root->startColumn,
          0,name,0,0,
          root->protection,Normal,FALSE,
          isMemberOf ? Foreign : isRelated ? Related : Member,
          MemberType_Enumeration,
          ArgumentList(),ArgumentList(),root->metaData) };
      md->setTagInfo(root->tagInfo());
      md->setLanguage(root->lang);
      md->setId(root->id);
      if (!isGlobal) md->setMemberClass(cd); else md->setFileDef(fd);
      md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
      md->setBodyDef(root->fileDef());
      md->setMemberSpecifiers(root->spec);
      md->setEnumBaseType(root->args);
      //printf("Enum %s definition at line %d of %s: protection=%d scope=%s\n",
      //    root->name.data(),root->bodyLine,root->fileName.data(),root->protection,cd?cd->name().data():"<none>");
      md->addSectionsToDefinition(root->anchors);
      md->setMemberGroupId(root->mGrpId);
      md->enableCallGraph(root->callGraph);
      md->enableCallerGraph(root->callerGraph);
      md->enableReferencedByRelation(root->referencedByRelation);
      md->enableReferencesRelation(root->referencesRelation);
      //printf("%s::setRefItems(%d)\n",md->name().data(),root->sli?root->sli->count():-1);
      md->setRefItems(root->sli);
      //printf("found enum %s nd=%p\n",md->name().data(),nd);
      bool defSet=FALSE;

      QCString baseType = root->args;
      if (!baseType.isEmpty())
      {
        baseType.prepend(" : ");
      }

      if (nd)
      {
        if (isRelated || Config_getBool(HIDE_SCOPE_NAMES))
        {
          md->setDefinition(name+baseType);
        }
        else
        {
          md->setDefinition(nd->name()+"::"+name+baseType);
        }
        //printf("definition=%s\n",md->definition());
        defSet=TRUE;
        md->setNamespace(nd);
        nd->insertMember(md.get());
      }

      // even if we have already added the enum to a namespace, we still
      // also want to add it to other appropriate places such as file
      // or class.
      if (isGlobal && (nd==0 || !nd->isAnonymous()))
      {
        if (!defSet) md->setDefinition(name+baseType);
        if (fd==0 && root->parent())
        {
          fd=root->parent()->fileDef();
        }
        if (fd)
        {
          md->setFileDef(fd);
          fd->insertMember(md.get());
        }
      }
      else if (cd)
      {
        if (isRelated || Config_getBool(HIDE_SCOPE_NAMES))
        {
          md->setDefinition(name+baseType);
        }
        else
        {
          md->setDefinition(cd->name()+"::"+name+baseType);
        }
        cd->insertMember(md.get());
        cd->insertUsedFile(fd);
      }
      md->setDocumentation(root->doc,root->docFile,root->docLine);
      md->setDocsForDefinition(!root->proto);
      md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
      md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);

      //printf("Adding member=%s\n",md->name().data());
      addMemberToGroups(root,md.get());

      MemberName *mn = mnsd->add(name);
      mn->push_back(std::move(md));
    }
  }
  else
  {
    for (const auto &e : root->children()) findEnums(e.get());
  }
}

//----------------------------------------------------------------------

static void addEnumValuesToEnums(const Entry *root)
{
  if (root->section==Entry::ENUM_SEC)
    // non anonymous enumeration
  {
    ClassDef       *cd=0;
    FileDef        *fd=0;
    NamespaceDef   *nd=0;
    MemberNameLinkedMap *mnsd=0;
    bool isGlobal;
    bool isRelated=FALSE;
    //printf("Found enum with name '%s' relates=%s\n",root->name.data(),root->relates.data());
    int i;

    QCString name;
    QCString scope;

    if ((i=root->name.findRev("::"))!=-1) // scope is specified
    {
      scope=root->name.left(i); // extract scope
      name=root->name.right(root->name.length()-i-2); // extract name
      if ((cd=getClass(scope))==0) nd=getResolvedNamespace(scope);
    }
    else // no scope, check the scope in which the docs where found
    {
      if (( root->parent()->section & Entry::SCOPE_MASK )
          && !root->parent()->name.isEmpty()
         ) // found enum docs inside a compound
      {
        scope=root->parent()->name;
        if ((cd=getClass(scope))==0) nd=getResolvedNamespace(scope);
      }
      name=root->name;
    }

    if (!root->relates.isEmpty())
    {   // related member, prefix user specified scope
      isRelated=TRUE;
      if (getClass(root->relates)==0 && !scope.isEmpty())
        scope=mergeScopes(scope,root->relates);
      else
        scope=root->relates.copy();
      if ((cd=getClass(scope))==0) nd=getResolvedNamespace(scope);
    }

    if (cd && !name.isEmpty()) // found a enum inside a compound
    {
      //printf("Enum in class '%s'::'%s'\n",cd->name().data(),name.data());
      fd=0;
      mnsd=Doxygen::memberNameLinkedMap;
      isGlobal=FALSE;
    }
    else if (nd && !nd->isAnonymous()) // found enum inside namespace
    {
      //printf("Enum in namespace '%s'::'%s'\n",nd->name().data(),name.data());
      mnsd=Doxygen::functionNameLinkedMap;
      isGlobal=TRUE;
    }
    else // found a global enum
    {
      fd=root->fileDef();
      //printf("Enum in file '%s': '%s'\n",fd->name().data(),name.data());
      mnsd=Doxygen::functionNameLinkedMap;
      isGlobal=TRUE;
    }

    if (!name.isEmpty())
    {
      //printf("** name=%s\n",name.data());
      MemberName *mn = mnsd->find(name); // for all members with this name
      if (mn)
      {
        struct EnumValueInfo
        {
          EnumValueInfo(const QCString &n,std::unique_ptr<MemberDef> &md) :
            name(n), member(std::move(md)) {}
          QCString name;
          std::unique_ptr<MemberDef> member;
        };
        std::vector< EnumValueInfo > extraMembers;
        // for each enum in this list
        for (const auto &md : *mn)
        {
          // use raw pointer in this loop, since we modify mn and can then invalidate mdp.
          if (!md->isAlias() && md->isEnumerate() && !root->children().empty())
          {
            //printf("   enum with %d children\n",root->children()->count());
            for (const auto &e : root->children())
            {
              SrcLangExt sle;
              if (
                   (sle=root->lang)==SrcLangExt_CSharp ||
                   sle==SrcLangExt_Java ||
                   sle==SrcLangExt_XML ||
                   (root->spec&Entry::Strong)
                 )
              {
                // Unlike classic C/C++ enums, for C++11, C# & Java enum
                // values are only visible inside the enum scope, so we must create
                // them here and only add them to the enum
                //printf("md->qualifiedName()=%s e->name=%s tagInfo=%p name=%s\n",
                //    md->qualifiedName().data(),e->name.data(),e->tagInfo,e->name.data());
                QCString qualifiedName = substitute(root->name,"::",".");
                if (!scope.isEmpty() && root->tagInfo())
                {
                  qualifiedName=substitute(scope,"::",".")+"."+qualifiedName;
                }
                if (substitute(md->qualifiedName(),"::",".")== // TODO: add function to get canonical representation
                    qualifiedName       // enum value scope matches that of the enum
                   )
                {
                  QCString fileName = e->fileName;
                  if (fileName.isEmpty() && e->tagInfo())
                  {
                    fileName = e->tagInfo()->tagName;
                  }
                  std::unique_ptr<MemberDef> fmd { createMemberDef(
                      fileName,e->startLine,e->startColumn,
                      e->type,e->name,e->args,0,
                      e->protection, Normal,e->stat,Member,
                      MemberType_EnumValue,ArgumentList(),ArgumentList(),e->metaData) };
                  if      (md->getClassDef())     fmd->setMemberClass(md->getClassDef());
                  else if (md->getNamespaceDef()) fmd->setNamespace(md->getNamespaceDef());
                  else if (md->getFileDef())      fmd->setFileDef(md->getFileDef());
                  fmd->setOuterScope(md->getOuterScope());
                  fmd->setTagInfo(e->tagInfo());
                  fmd->setLanguage(e->lang);
                  fmd->setId(e->id);
                  fmd->setDocumentation(e->doc,e->docFile,e->docLine);
                  fmd->setBriefDescription(e->brief,e->briefFile,e->briefLine);
                  fmd->addSectionsToDefinition(e->anchors);
                  fmd->setInitializer(e->initializer);
                  fmd->setMaxInitLines(e->initLines);
                  fmd->setMemberGroupId(e->mGrpId);
                  fmd->setExplicitExternal(e->explicitExternal,fileName,e->startLine,e->startColumn);
                  fmd->setRefItems(e->sli);
                  fmd->setAnchor();
                  md->insertEnumField(fmd.get());
                  fmd->setEnumScope(md.get(),TRUE);
                  extraMembers.push_back(EnumValueInfo(e->name,fmd));
                }
              }
              else
              {
                //printf("e->name=%s isRelated=%d\n",e->name.data(),isRelated);
                MemberName *fmn=0;
                MemberNameLinkedMap *emnsd = isRelated ? Doxygen::functionNameLinkedMap : mnsd;
                if (!e->name.isEmpty() && (fmn=emnsd->find(e->name)))
                  // get list of members with the same name as the field
                {
                  for (const auto &fmd : *fmn)
                  {
                    if (fmd->isEnumValue() && fmd->getOuterScope()==md->getOuterScope()) // in same scope
                    {
                      //printf("found enum value with same name %s in scope %s\n",
                      //    fmd->name().data(),fmd->getOuterScope()->name().data());
                      if (nd && !nd->isAnonymous())
                      {
                        const NamespaceDef *fnd=fmd->getNamespaceDef();
                        if (fnd==nd) // enum value is inside a namespace
                        {
                          md->insertEnumField(fmd.get());
                          fmd->setEnumScope(md.get());
                        }
                      }
                      else if (isGlobal)
                      {
                        const FileDef *ffd=fmd->getFileDef();
                        if (ffd==fd) // enum value has file scope
                        {
                          md->insertEnumField(fmd.get());
                          fmd->setEnumScope(md.get());
                        }
                      }
                      else if (isRelated && cd) // reparent enum value to
                                                // match the enum's scope
                      {
                        md->insertEnumField(fmd.get());   // add field def to list
                        fmd->setEnumScope(md.get());      // cross ref with enum name
                        fmd->setEnumClassScope(cd); // cross ref with enum name
                        fmd->setOuterScope(cd);
                        fmd->makeRelated();
                        cd->insertMember(fmd.get());
                      }
                      else
                      {
                        const ClassDef *fcd=fmd->getClassDef();
                        if (fcd==cd) // enum value is inside a class
                        {
                          //printf("Inserting enum field %s in enum scope %s\n",
                          //    fmd->name().data(),md->name().data());
                          md->insertEnumField(fmd.get()); // add field def to list
                          fmd->setEnumScope(md.get());    // cross ref with enum name
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
        // move the newly added members into mn
        for (auto &e : extraMembers)
        {
          MemberName *emn=mnsd->add(e.name);
          emn->push_back(std::move(e.member));
        }
      }
    }
  }
  else
  {
    for (const auto &e : root->children()) addEnumValuesToEnums(e.get());
  }
}


//----------------------------------------------------------------------
// find the documentation blocks for the enumerations

static void findEnumDocumentation(const Entry *root)
{
  if (root->section==Entry::ENUMDOC_SEC
      && !root->name.isEmpty()
      && root->name.at(0)!='@'        // skip anonymous enums
     )
  {
    //printf("Found docs for enum with name '%s' in context %s\n",
    //    root->name.data(),root->parent->name.data());
    int i;
    QCString name;
    QCString scope;
    if ((i=root->name.findRev("::"))!=-1) // scope is specified as part of the name
    {
      name=root->name.right(root->name.length()-i-2); // extract name
      scope=root->name.left(i); // extract scope
      //printf("Scope='%s' Name='%s'\n",scope.data(),name.data());
    }
    else // just the name
    {
      name=root->name;
    }
    if (( root->parent()->section & Entry::SCOPE_MASK )
        && !root->parent()->name.isEmpty()
       ) // found enum docs inside a compound
    {
      if (!scope.isEmpty()) scope.prepend("::");
      scope.prepend(root->parent()->name);
    }
    ClassDef *cd=getClass(scope);

    if (!name.isEmpty())
    {
      bool found=FALSE;
      if (cd)
      {
        //printf("Enum: scope='%s' name='%s'\n",cd->name(),name.data());
        QCString className=cd->name().copy();
        MemberName *mn=Doxygen::memberNameLinkedMap->find(name);
        if (mn)
        {
          for (const auto &md : *mn)
          {
            cd=md->getClassDef();
            if (cd && cd->name()==className && md->isEnumerate())
            {
              // documentation outside a compound overrides the documentation inside it
#if 0
              if (!md->documentation() || root->parent()->name.isEmpty())
#endif
              {
                md->setDocumentation(root->doc,root->docFile,root->docLine);
                md->setDocsForDefinition(!root->proto);
              }

              // brief descriptions inside a compound override the documentation
              // outside it
#if 0
              if (!md->briefDescription() || !root->parent()->name.isEmpty())
#endif
              {
                md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
              }

              if (!md->inbodyDocumentation() || !root->parent()->name.isEmpty())
              {
                md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
              }

              if (root->mGrpId!=-1 && md->getMemberGroupId()==-1)
              {
                md->setMemberGroupId(root->mGrpId);
              }

              md->addSectionsToDefinition(root->anchors);
              md->setRefItems(root->sli);

              const GroupDef *gd=md->getGroupDef();
              if (gd==0 && !root->groups.empty()) // member not grouped but out-of-line documentation is
              {
                addMemberToGroups(root,md.get());
              }

              found=TRUE;
              break;
            }
          }
        }
        else
        {
          //printf("MemberName %s not found!\n",name.data());
        }
      }
      else // enum outside class
      {
        //printf("Enum outside class: %s grpId=%d\n",name.data(),root->mGrpId);
        MemberName *mn=Doxygen::functionNameLinkedMap->find(name);
        if (mn)
        {
          for (const auto &md : *mn)
          {
            if (md->isEnumerate())
            {
              md->setDocumentation(root->doc,root->docFile,root->docLine);
              md->setDocsForDefinition(!root->proto);
              md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
              md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
              md->addSectionsToDefinition(root->anchors);
              md->setMemberGroupId(root->mGrpId);

              const GroupDef *gd=md->getGroupDef();
              if (gd==0 && !root->groups.empty()) // member not grouped but out-of-line documentation is
              {
                addMemberToGroups(root,md.get());
              }

              found=TRUE;
              break;
            }
          }
        }
      }
      if (!found)
      {
        warn(root->fileName,root->startLine,
             "Documentation for undefined enum '%s' found.",
             name.data()
            );
      }
    }
  }
  for (const auto &e : root->children()) findEnumDocumentation(e.get());
}

// search for each enum (member or function) in mnl if it has documented
// enum values.
static void findDEV(const MemberNameLinkedMap &mnsd)
{
  // for each member name
  for (const auto &mn : mnsd)
  {
    // for each member definition
    for (const auto &md : *mn)
    {
      if (md->isEnumerate()) // member is an enum
      {
        const MemberList *fmdl = md->enumFieldList();
        int documentedEnumValues=0;
        if (fmdl) // enum has values
        {
          MemberListIterator fmni(*fmdl);
          MemberDef *fmd;
          // for each enum value
          for (fmni.toFirst();(fmd=fmni.current());++fmni)
          {
            if (fmd->isLinkableInProject()) documentedEnumValues++;
          }
        }
        // at least one enum value is documented
        if (documentedEnumValues>0) md->setDocumentedEnumValues(TRUE);
      }
    }
  }
}

// search for each enum (member or function) if it has documented enum
// values.
static void findDocumentedEnumValues()
{
  findDEV(*Doxygen::memberNameLinkedMap);
  findDEV(*Doxygen::functionNameLinkedMap);
}

//----------------------------------------------------------------------

static void addMembersToIndex()
{
  // for each member name
  for (const auto &mn : *Doxygen::memberNameLinkedMap)
  {
    // for each member definition
    for (const auto &md : *mn)
    {
      addClassMemberNameToIndex(md.get());
    }
  }
  // for each member name
  for (const auto &mn : *Doxygen::functionNameLinkedMap)
  {
    // for each member definition
    for (const auto &md : *mn)
    {
      if (!md->isAlias())
      {
        if (md->getNamespaceDef())
        {
          addNamespaceMemberNameToIndex(md.get());
        }
        else
        {
          addFileMemberNameToIndex(md.get());
        }
      }
    }
  }
}

//----------------------------------------------------------------------

static void vhdlCorrectMemberProperties()
{
  // for each member name
  for (const auto &mn : *Doxygen::memberNameLinkedMap)
  {
    // for each member definition
    for (const auto &md : *mn)
    {
      VhdlDocGen::correctMemberProperties(md.get());
    }
  }
  // for each member name
  for (const auto &mn : *Doxygen::functionNameLinkedMap)
  {
    // for each member definition
    for (const auto &md : *mn)
    {
      VhdlDocGen::correctMemberProperties(md.get());
    }
  }
}


//----------------------------------------------------------------------
// computes the relation between all members. For each member 'm'
// the members that override the implementation of 'm' are searched and
// the member that 'm' overrides is searched.

static void computeMemberRelations()
{
  for (const auto &mn : *Doxygen::memberNameLinkedMap)
  {
    // for each member with a specific name
    for (const auto &md : *mn)
    {
      // for each other member with the same name
      for ( const auto &bmd : *mn)
      {
        if (md!=bmd)
        {
          const ClassDef *mcd  = md->getClassDef();
          if (mcd && !mcd->baseClasses().empty())
          {
            const ClassDef *bmcd = bmd->getClassDef();
            //printf("Check relation between '%s'::'%s' (%p) and '%s'::'%s' (%p)\n",
            //       mcd->name().data(),md->name().data(),md.get(),
            //       bmcd->name().data(),bmd->name().data(),bmd.get()
            //      );
            if (bmcd && mcd && bmcd!=mcd &&
                (bmd->virtualness()!=Normal ||
                 bmd->getLanguage()==SrcLangExt_Python || bmd->getLanguage()==SrcLangExt_Java || bmd->getLanguage()==SrcLangExt_PHP ||
                 bmcd->compoundType()==ClassDef::Interface || bmcd->compoundType()==ClassDef::Protocol
                ) &&
                md->isFunction() &&
                mcd->isLinkable() &&
                bmcd->isLinkable() &&
                mcd->isBaseClass(bmcd,TRUE))
            {
              //printf("  derived scope\n");
              const ArgumentList &bmdAl = bmd->argumentList();
              const ArgumentList &mdAl =  md->argumentList();
              //printf(" Base argList='%s'\n Super argList='%s'\n",
              //        argListToString(bmdAl).data(),
              //        argListToString(mdAl).data()
              //      );
              if (
                  matchArguments2(bmd->getOuterScope(),bmd->getFileDef(),&bmdAl,
                    md->getOuterScope(), md->getFileDef(), &mdAl,
                    TRUE
                    )
                 )
              {
                //printf("match!\n");
                MemberDef *rmd;
                if ((rmd=md->reimplements())==0 ||
                    minClassDistance(mcd,bmcd)<minClassDistance(mcd,rmd->getClassDef())
                   )
                {
                  //printf("setting (new) reimplements member\n");
                  md->setReimplements(bmd.get());
                }
                //printf("%s: add reimplementedBy member %s\n",bmcd->name().data(),mcd->name().data());
                bmd->insertReimplementedBy(md.get());
              }
              else
              {
                //printf("no match!\n");
              }
            }
          }
        }
      }
    }
  }
}


//----------------------------------------------------------------------------
//static void computeClassImplUsageRelations()
//{
//  ClassDef *cd;
//  ClassSDict::Iterator cli(*Doxygen::classSDict);
//  for (;(cd=cli.current());++cli)
//  {
//    cd->determineImplUsageRelation();
//  }
//}

//----------------------------------------------------------------------------

static void createTemplateInstanceMembers()
{
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  ClassDef *cd;
  // for each class
  for (cli.toFirst();(cd=cli.current());++cli)
  {
    // that is a template
    QDict<ClassDef> *templInstances = cd->getTemplateInstances();
    if (templInstances)
    {
      QDictIterator<ClassDef> qdi(*templInstances);
      ClassDef *tcd=0;
      // for each instance of the template
      for (qdi.toFirst();(tcd=qdi.current());++qdi)
      {
        tcd->addMembersToTemplateInstance(cd,qdi.currentKey());
      }
    }
  }
}

//----------------------------------------------------------------------------

static void mergeCategories()
{
  ClassDef *cd;
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  // merge members of categories into the class they extend
  for (cli.toFirst();(cd=cli.current());++cli)
  {
    int i=cd->name().find('(');
    if (i!=-1) // it is an Objective-C category
    {
      QCString baseName=cd->name().left(i);
      ClassDef *baseClass=Doxygen::classSDict->find(baseName);
      if (baseClass)
      {
        //printf("*** merging members of category %s into %s\n",
        //    cd->name().data(),baseClass->name().data());
        baseClass->mergeCategory(cd);
      }
    }
  }
}

// builds the list of all members for each class

static void buildCompleteMemberLists()
{
  ClassDef *cd;
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  // merge the member list of base classes into the inherited classes.
  for (cli.toFirst();(cd=cli.current());++cli)
  {
    if (// !cd->isReference() && // not an external class
         cd->subClasses().empty() && // is a root of the hierarchy
         !cd->baseClasses().empty()) // and has at least one base class
    {
      //printf("*** merging members for %s\n",cd->name().data());
      cd->mergeMembers();
    }
  }
  // now sort the member list of all members for all classes.
  for (cli.toFirst();(cd=cli.current());++cli)
  {
    cd->sortAllMembersList();
  }
}

//----------------------------------------------------------------------------

static void generateFileSources()
{
  if (!Doxygen::inputNameLinkedMap->empty())
  {
#if USE_LIBCLANG
    if (Doxygen::clangAssistedParsing)
    {
      StringUnorderedSet processedFiles;

      // create a dictionary with files to process
      StringUnorderedSet filesToProcess;

      for (const auto &fn : *Doxygen::inputNameLinkedMap)
      {
        for (const auto &fd : *fn)
        {
          filesToProcess.insert(fd->absFilePath().str());
        }
      }
      // process source files (and their include dependencies)
      for (const auto &fn : *Doxygen::inputNameLinkedMap)
      {
        for (const auto &fd : *fn)
        {
          if (fd->isSource() && !fd->isReference() &&
              ((fd->generateSourceFile() && !g_useOutputTemplate) ||
               (!fd->isReference() && Doxygen::parseSourcesNeeded)
              )
             )
          {
            auto clangParser = ClangParser::instance()->createTUParser(fd.get());
            if (fd->generateSourceFile() && !g_useOutputTemplate) // sources need to be shown in the output
            {
              msg("Generating code for file %s...\n",fd->docName().data());
              clangParser->parse();
              fd->writeSourceHeader(*g_outputList);
              fd->writeSourceBody(*g_outputList,clangParser.get());
              fd->writeSourceFooter(*g_outputList);
            }
            else if (!fd->isReference() && Doxygen::parseSourcesNeeded)
              // we needed to parse the sources even if we do not show them
            {
              msg("Parsing code for file %s...\n",fd->docName().data());
              clangParser->parse();
              fd->parseSource(clangParser.get());
            }

            for (auto incFile : clangParser->filesInSameTU())
            {
              if (filesToProcess.find(incFile)!=filesToProcess.end() &&  // part of input
                  fd->absFilePath()!=incFile &&                          // not same file
                  processedFiles.find(incFile)==processedFiles.end())    // not yet marked as processed
              {
                StringVector moreFiles;
                bool ambig;
                FileDef *ifd=findFileDef(Doxygen::inputNameLinkedMap,incFile.c_str(),ambig);
                if (ifd && !ifd->isReference())
                {
                  if (ifd->generateSourceFile() && !g_useOutputTemplate) // sources need to be shown in the output
                  {
                    msg(" Generating code for file %s...\n",ifd->docName().data());
                    ifd->writeSourceHeader(*g_outputList);
                    ifd->writeSourceBody(*g_outputList,clangParser.get());
                    ifd->writeSourceFooter(*g_outputList);
                  }
                  else if (!ifd->isReference() && Doxygen::parseSourcesNeeded)
                    // we needed to parse the sources even if we do not show them
                  {
                    msg(" Parsing code for file %s...\n",ifd->docName().data());
                    ifd->parseSource(clangParser.get());
                  }
                  processedFiles.insert(incFile);
                }
              }
            }
            processedFiles.insert(fd->absFilePath().str());
          }
        }
      }
      // process remaining files
      for (const auto &fn : *Doxygen::inputNameLinkedMap)
      {
        for (const auto &fd : *fn)
        {
          if (processedFiles.find(fd->absFilePath().str())==processedFiles.end()) // not yet processed
          {
            if (fd->generateSourceFile() && !Htags::useHtags && !g_useOutputTemplate) // sources need to be shown in the output
            {
              auto clangParser = ClangParser::instance()->createTUParser(fd.get());
              msg("Generating code for file %s...\n",fd->docName().data());
              clangParser->parse();
              fd->writeSourceHeader(*g_outputList);
              fd->writeSourceBody(*g_outputList,clangParser.get());
              fd->writeSourceFooter(*g_outputList);
            }
            else if (!fd->isReference() && Doxygen::parseSourcesNeeded)
              // we needed to parse the sources even if we do not show them
            {
              auto clangParser = ClangParser::instance()->createTUParser(fd.get());
              msg("Parsing code for file %s...\n",fd->docName().data());
              clangParser->parse();
              fd->writeSourceHeader(*g_outputList);
              fd->writeSourceBody(*g_outputList,clangParser.get());
              fd->writeSourceFooter(*g_outputList);
            }
          }
        }
      }
    }
    else
#endif
    {
#define MULTITHREADED_SOURCE_GENERATOR 0 // not ready to be enabled yet
#if MULTITHREADED_SOURCE_GENERATOR
      std::size_t numThreads = static_cast<std::size_t>(Config_getInt(NUM_PROC_THREADS));
      if (numThreads==0)
      {
        numThreads = std::thread::hardware_concurrency();
      }
      msg("Generating code files using %zu threads.\n",numThreads);
      struct SourceContext
      {
        SourceContext(FileDef *fd_,bool gen_,OutputList ol_)
          : fd(fd_), generateSourceFile(gen_), ol(ol_) {}
        FileDef *fd;
        bool generateSourceFile;
        OutputList ol;
      };
      ThreadPool threadPool(numThreads);
      std::vector< std::future< std::shared_ptr<SourceContext> > > results;
      for (const auto &fn : *Doxygen::inputNameLinkedMap)
      {
        for (const auto &fd : *fn)
        {
          bool generateSourceFile = fd->generateSourceFile() && !Htags::useHtags && !g_useOutputTemplate;
          auto ctx = std::make_shared<SourceContext>(fd.get(),generateSourceFile,*g_outputList);
          if (generateSourceFile)
          {
            msg("Generating code for file %s...\n",fd->docName().data());
            fd->writeSourceHeader(ctx->ol);
          }
          else
          {
            msg("Parsing code for file %s...\n",fd->docName().data());
          }
          auto processFile = [ctx]() {
            StringVector filesInSameTu;
            ctx->fd->getAllIncludeFilesRecursively(filesInSameTu);
            if (ctx->generateSourceFile) // sources need to be shown in the output
            {
              ctx->fd->writeSourceBody(ctx->ol,nullptr);
            }
            else if (!ctx->fd->isReference() && Doxygen::parseSourcesNeeded)
              // we needed to parse the sources even if we do not show them
            {
              ctx->fd->parseSource(nullptr);
            }
            return ctx;
          };
          results.emplace_back(threadPool.queue(processFile));
        }
      }
      // first wait until all files are processed
      for (auto &f : results)
      {
        auto ctx = f.get();
        if (ctx->generateSourceFile)
        {
          ctx->fd->writeSourceFooter(ctx->ol);
        }
      }
#else // single threaded version
      for (const auto &fn : *Doxygen::inputNameLinkedMap)
      {
        for (const auto &fd : *fn)
        {
          StringVector filesInSameTu;
          fd->getAllIncludeFilesRecursively(filesInSameTu);
          if (fd->generateSourceFile() && !Htags::useHtags && !g_useOutputTemplate) // sources need to be shown in the output
          {
            msg("Generating code for file %s...\n",fd->docName().data());
            fd->writeSourceHeader(*g_outputList);
            fd->writeSourceBody(*g_outputList,nullptr);
            fd->writeSourceFooter(*g_outputList);
          }
          else if (!fd->isReference() && Doxygen::parseSourcesNeeded)
            // we needed to parse the sources even if we do not show them
          {
            msg("Parsing code for file %s...\n",fd->docName().data());
            fd->parseSource(nullptr);
          }
        }
      }
#endif
    }
  }
}

//----------------------------------------------------------------------------

static void generateFileDocs()
{
  if (documentedHtmlFiles==0) return;

  if (!Doxygen::inputNameLinkedMap->empty())
  {
    for (const auto &fn : *Doxygen::inputNameLinkedMap)
    {
      for (const auto &fd : *fn)
      {
        bool doc = fd->isLinkableInProject();
        if (doc)
        {
          msg("Generating docs for file %s...\n",fd->docName().data());
          fd->writeDocumentation(*g_outputList);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------

static void addSourceReferences()
{
  // add source references for class definitions
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  ClassDef *cd=0;
  for (cli.toFirst();(cd=cli.current());++cli)
  {
    FileDef *fd=cd->getBodyDef();
    if (fd && cd->isLinkableInProject() && cd->getStartDefLine()!=-1)
    {
      fd->addSourceRef(cd->getStartDefLine(),cd,0);
    }
  }
  // add source references for namespace definitions
  NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
  NamespaceDef *nd=0;
  for (nli.toFirst();(nd=nli.current());++nli)
  {
    FileDef *fd=nd->getBodyDef();
    if (fd && nd->isLinkableInProject() && nd->getStartDefLine()!=-1)
    {
      fd->addSourceRef(nd->getStartDefLine(),nd,0);
    }
  }

  // add source references for member names
  for (const auto &mn : *Doxygen::memberNameLinkedMap)
  {
    for (const auto &md : *mn)
    {
      //printf("class member %s: def=%s body=%d link?=%d\n",
      //    md->name().data(),
      //    md->getBodyDef()?md->getBodyDef()->name().data():"<none>",
      //    md->getStartBodyLine(),md->isLinkableInProject());
      FileDef *fd=md->getBodyDef();
      if (fd &&
          md->getStartDefLine()!=-1 &&
          md->isLinkableInProject() &&
          (fd->generateSourceFile() || Doxygen::parseSourcesNeeded)
         )
      {
        //printf("Found member '%s' in file '%s' at line '%d' def=%s\n",
        //    md->name().data(),fd->name().data(),md->getStartBodyLine(),md->getOuterScope()->name().data());
        fd->addSourceRef(md->getStartDefLine(),md->getOuterScope(),md.get());
      }
    }
  }
  for (const auto &mn : *Doxygen::functionNameLinkedMap)
  {
    for (const auto &md : *mn)
    {
      FileDef *fd=md->getBodyDef();
      //printf("member %s body=[%d,%d] fd=%p link=%d parseSources=%d\n",
      //    md->name().data(),
      //    md->getStartBodyLine(),md->getEndBodyLine(),fd,
      //    md->isLinkableInProject(),
      //    Doxygen::parseSourcesNeeded);
      if (fd &&
          md->getStartDefLine()!=-1 &&
          md->isLinkableInProject() &&
          (fd->generateSourceFile() || Doxygen::parseSourcesNeeded)
         )
      {
        //printf("Found member '%s' in file '%s' at line '%d' def=%s\n",
        //    md->name().data(),fd->name().data(),md->getStartBodyLine(),md->getOuterScope()->name().data());
        fd->addSourceRef(md->getStartDefLine(),md->getOuterScope(),md.get());
      }
    }
  }
}

//----------------------------------------------------------------------------

// add the macro definitions found during preprocessing as file members
static void buildDefineList()
{
  for (const auto &s : g_inputFiles)
  {
    auto it = Doxygen::macroDefinitions.find(s);
    if (it!=Doxygen::macroDefinitions.end())
    {
      for (const auto &def : it->second)
      {
        std::unique_ptr<MemberDef> md { createMemberDef(
            def.fileName,def.lineNr,def.columnNr,
            "#define",def.name,def.args,0,
            Public,Normal,FALSE,Member,MemberType_Define,
            ArgumentList(),ArgumentList(),"") };

        if (!def.args.isEmpty())
        {
          md->moveArgumentList(stringToArgumentList(SrcLangExt_Cpp, def.args));
        }
        md->setInitializer(def.definition);
        md->setFileDef(def.fileDef);
        md->setDefinition("#define "+def.name);

        MemberName *mn=Doxygen::functionNameLinkedMap->add(def.name);
        if (def.fileDef)
        {
          def.fileDef->insertMember(md.get());
        }
        mn->push_back(std::move(md));
      }
    }
  }
}

//----------------------------------------------------------------------------

static void sortMemberLists()
{
  // sort class member lists
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  ClassDef *cd=0;
  for (cli.toFirst();(cd=cli.current());++cli)
  {
    cd->sortMemberLists();
  }

  // sort namespace member lists
  NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
  NamespaceDef *nd=0;
  for (nli.toFirst();(nd=nli.current());++nli)
  {
    nd->sortMemberLists();
  }

  // sort file member lists
  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    for (const auto &fd : *fn)
    {
      fd->sortMemberLists();
    }
  }

  // sort group member lists
  GroupSDict::Iterator gli(*Doxygen::groupSDict);
  GroupDef *gd;
  for (gli.toFirst();(gd=gli.current());++gli)
  {
    gd->sortMemberLists();
  }
}

//----------------------------------------------------------------------------

static void setAnonymousEnumType()
{
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  ClassDef *cd=0;
  for (cli.toFirst();(cd=cli.current());++cli)
  {
    cd->setAnonymousEnumType();
  }
}

//----------------------------------------------------------------------------

static void countMembers()
{
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  ClassDef *cd=0;
  for (cli.toFirst();(cd=cli.current());++cli)
  {
    cd->countMembers();
  }

  NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
  NamespaceDef *nd=0;
  for (nli.toFirst();(nd=nli.current());++nli)
  {
    nd->countMembers();
  }

  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    for (const auto &fd : *fn)
    {
      fd->countMembers();
    }
  }

  GroupSDict::Iterator gli(*Doxygen::groupSDict);
  GroupDef *gd;
  for (gli.toFirst();(gd=gli.current());++gli)
  {
    gd->countMembers();
  }
}


//----------------------------------------------------------------------------
// generate the documentation of all classes

static void generateClassList(ClassSDict &classSDict)
{
  ClassSDict::Iterator cli(classSDict);
  for ( ; cli.current() ; ++cli )
  {
    ClassDef *cd=cli.current();

    //printf("cd=%s getOuterScope=%p global=%p\n",cd->name().data(),cd->getOuterScope(),Doxygen::globalScope);
    if (cd &&
        (cd->getOuterScope()==0 || // <-- should not happen, but can if we read an old tag file
         cd->getOuterScope()==Doxygen::globalScope // only look at global classes
        ) && !cd->isHidden() && !cd->isEmbeddedInOuterScope()
       )
    {
      // skip external references, anonymous compounds and
      // template instances
      if ( cd->isLinkableInProject() && cd->templateMaster()==0)
      {
        msg("Generating docs for compound %s...\n",cd->name().data());

        cd->writeDocumentation(*g_outputList);
        cd->writeMemberList(*g_outputList);
      }
      // even for undocumented classes, the inner classes can be documented.
      cd->writeDocumentationForInnerClasses(*g_outputList);
    }
  }
}

static void generateClassDocs()
{
  generateClassList(*Doxygen::classSDict);
  generateClassList(*Doxygen::hiddenClasses);
}

//----------------------------------------------------------------------------

static void inheritDocumentation()
{
  for (const auto &mn : *Doxygen::memberNameLinkedMap)
  {
    for (const auto &md : *mn)
    {
      //static int count=0;
      //printf("%04d Member '%s'\n",count++,md->qualifiedName().data());
      if (md->documentation().isEmpty() && md->briefDescription().isEmpty())
      { // no documentation yet
        MemberDef *bmd = md->reimplements();
        while (bmd && bmd->documentation().isEmpty() &&
                      bmd->briefDescription().isEmpty()
              )
        { // search up the inheritance tree for a documentation member
          //printf("bmd=%s class=%s\n",bmd->name().data(),bmd->getClassDef()->name().data());
          bmd = bmd->reimplements();
        }
        if (bmd) // copy the documentation from the reimplemented member
        {
          md->setInheritsDocsFrom(bmd);
          md->setDocumentation(bmd->documentation(),bmd->docFile(),bmd->docLine());
          md->setDocsForDefinition(bmd->isDocsForDefinition());
          md->setBriefDescription(bmd->briefDescription(),bmd->briefFile(),bmd->briefLine());
          md->copyArgumentNames(bmd);
          md->setInbodyDocumentation(bmd->inbodyDocumentation(),bmd->inbodyFile(),bmd->inbodyLine());
        }
      }
    }
  }
}

//----------------------------------------------------------------------------

static void combineUsingRelations()
{
  // for each file
  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    for (const auto &fd : *fn)
    {
      fd->setVisited(FALSE);
    }
  }
  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    for (const auto &fd : *fn)
    {
      fd->combineUsingRelations();
    }
  }

  // for each namespace
  NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
  NamespaceDef *nd;
  for (nli.toFirst() ; (nd=nli.current()) ; ++nli )
  {
    nd->setVisited(FALSE);
  }
  for (nli.toFirst() ; (nd=nli.current()) ; ++nli )
  {
    nd->combineUsingRelations();
  }
}

//----------------------------------------------------------------------------

static void addMembersToMemberGroup()
{
  // for each class
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  ClassDef *cd;
  for ( ; (cd=cli.current()) ; ++cli )
  {
    cd->addMembersToMemberGroup();
  }
  // for each file
  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    for (const auto &fd : *fn)
    {
      fd->addMembersToMemberGroup();
    }
  }
  // for each namespace
  NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
  NamespaceDef *nd;
  for ( ; (nd=nli.current()) ; ++nli )
  {
    nd->addMembersToMemberGroup();
  }
  // for each group
  GroupSDict::Iterator gli(*Doxygen::groupSDict);
  GroupDef *gd;
  for (gli.toFirst();(gd=gli.current());++gli)
  {
    gd->addMembersToMemberGroup();
  }
}

//----------------------------------------------------------------------------

static void distributeMemberGroupDocumentation()
{
  // for each class
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  ClassDef *cd;
  for ( ; (cd=cli.current()) ; ++cli )
  {
    cd->distributeMemberGroupDocumentation();
  }
  // for each file
  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    for (const auto &fd : *fn)
    {
      fd->distributeMemberGroupDocumentation();
    }
  }
  // for each namespace
  NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
  NamespaceDef *nd;
  for ( ; (nd=nli.current()) ; ++nli )
  {
    nd->distributeMemberGroupDocumentation();
  }
  // for each group
  GroupSDict::Iterator gli(*Doxygen::groupSDict);
  GroupDef *gd;
  for (gli.toFirst();(gd=gli.current());++gli)
  {
    gd->distributeMemberGroupDocumentation();
  }
}

//----------------------------------------------------------------------------

static void findSectionsInDocumentation()
{
  // for each class
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  ClassDef *cd;
  for ( ; (cd=cli.current()) ; ++cli )
  {
    cd->findSectionsInDocumentation();
  }
  // for each file
  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    for (const auto &fd : *fn)
    {
      fd->findSectionsInDocumentation();
    }
  }
  // for each namespace
  NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
  NamespaceDef *nd;
  for ( ; (nd=nli.current()) ; ++nli )
  {
    nd->findSectionsInDocumentation();
  }
  // for each group
  GroupSDict::Iterator gli(*Doxygen::groupSDict);
  GroupDef *gd;
  for (gli.toFirst();(gd=gli.current());++gli)
  {
    gd->findSectionsInDocumentation();
  }
  // for each page
  PageSDict::Iterator pdi(*Doxygen::pageSDict);
  PageDef *pd=0;
  for (pdi.toFirst();(pd=pdi.current());++pdi)
  {
    pd->findSectionsInDocumentation();
  }
  if (Doxygen::mainPage) Doxygen::mainPage->findSectionsInDocumentation();
}

static void flushCachedTemplateRelations()
{
  // remove all references to classes from the cache
  // as there can be new template instances in the inheritance path
  // to this class. Optimization: only remove those classes that
  // have inheritance instances as direct or indirect sub classes.
  StringVector elementsToRemove;
  for (const auto &ci : *Doxygen::lookupCache)
  {
    const LookupInfo &li = ci.second;
    if (li.classDef)
    {
      elementsToRemove.push_back(ci.first);
    }
  }
  for (const auto &k : elementsToRemove)
  {
    Doxygen::lookupCache->remove(k);
  }

  // remove all cached typedef resolutions whose target is a
  // template class as this may now be a template instance
  // for each global function name
  for (const auto &fn : *Doxygen::functionNameLinkedMap)
  {
    // for each function with that name
    for (const auto &fmd : *fn)
    {
      if (fmd->isTypedefValCached())
      {
        const ClassDef *cd = fmd->getCachedTypedefVal();
        if (cd->isTemplate()) fmd->invalidateTypedefValCache();
      }
    }
  }
  // for each class method name
  for (const auto &nm : *Doxygen::memberNameLinkedMap)
  {
    // for each function with that name
    for (const auto &md : *nm)
    {
      if (md->isTypedefValCached())
      {
        const ClassDef *cd = md->getCachedTypedefVal();
        if (cd->isTemplate()) md->invalidateTypedefValCache();
      }
    }
  }
}

//----------------------------------------------------------------------------

static void flushUnresolvedRelations()
{
  // Remove all unresolved references to classes from the cache.
  // This is needed before resolving the inheritance relations, since
  // it would otherwise not find the inheritance relation
  // for C in the example below, as B::I was already found to be unresolvable
  // (which is correct if you ignore the inheritance relation between A and B).
  //
  // class A { class I {} };
  // class B : public A {};
  // class C : public B::I {};

  StringVector elementsToRemove;
  for (const auto &ci : *Doxygen::lookupCache)
  {
    const LookupInfo &li = ci.second;
    if (li.classDef==0 && li.typeDef==0)
    {
      elementsToRemove.push_back(ci.first);
    }
  }
  for (const auto &k : elementsToRemove)
  {
    Doxygen::lookupCache->remove(k);
  }

  // for each global function name
  for (const auto &fn : *Doxygen::functionNameLinkedMap)
  {
    // for each function with that name
    for (const auto &fmd : *fn)
    {
      fmd->invalidateCachedArgumentTypes();
    }
  }
  // for each class method name
  for (const auto &nm : *Doxygen::memberNameLinkedMap)
  {
    // for each function with that name
    for (const auto &md : *nm)
    {
      md->invalidateCachedArgumentTypes();
    }
  }

}

//----------------------------------------------------------------------------

static void findDefineDocumentation(Entry *root)
{
  if ((root->section==Entry::DEFINEDOC_SEC ||
       root->section==Entry::DEFINE_SEC) && !root->name.isEmpty()
     )
  {
    //printf("found define '%s' '%s' brief='%s' doc='%s'\n",
    //       root->name.data(),root->args.data(),root->brief.data(),root->doc.data());

    if (root->tagInfo() && !root->name.isEmpty()) // define read from a tag file
    {
      std::unique_ptr<MemberDef> md { createMemberDef(root->tagInfo()->tagName,1,1,
                    "#define",root->name,root->args,0,
                    Public,Normal,FALSE,Member,MemberType_Define,
                    ArgumentList(),ArgumentList(),"") };
      md->setTagInfo(root->tagInfo());
      md->setLanguage(root->lang);
      //printf("Searching for '%s' fd=%p\n",filePathName.data(),fd);
      md->setFileDef(root->parent()->fileDef());
      //printf("Adding member=%s\n",md->name().data());
      MemberName *mn = Doxygen::functionNameLinkedMap->add(root->name);
      mn->push_back(std::move(md));
    }
    MemberName *mn=Doxygen::functionNameLinkedMap->find(root->name);
    if (mn)
    {
      int count=0;
      for (const auto &md : *mn)
      {
        if (md->memberType()==MemberType_Define) count++;
      }
      if (count==1)
      {
        for (const auto &md : *mn)
        {
          if (md->memberType()==MemberType_Define)
          {
            md->setDocumentation(root->doc,root->docFile,root->docLine);
            md->setDocsForDefinition(!root->proto);
            md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
            if (md->inbodyDocumentation().isEmpty())
            {
              md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
            }
            md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
            md->setBodyDef(root->fileDef());
            md->addSectionsToDefinition(root->anchors);
            md->setMaxInitLines(root->initLines);
            md->setRefItems(root->sli);
            if (root->mGrpId!=-1) md->setMemberGroupId(root->mGrpId);
            addMemberToGroups(root,md.get());
          }
        }
      }
      else if (count>1 &&
               (!root->doc.isEmpty() ||
                !root->brief.isEmpty() ||
                root->bodyLine!=-1
               )
              )
        // multiple defines don't know where to add docs
        // but maybe they are in different files together with their documentation
      {
        for (const auto &md : *mn)
        {
          if (md->memberType()==MemberType_Define)
          {
            const FileDef *fd=md->getFileDef();
            if (fd && fd->absFilePath()==root->fileName)
              // doc and define in the same file assume they belong together.
            {
#if 0
              if (md->documentation().isEmpty())
#endif
              {
                md->setDocumentation(root->doc,root->docFile,root->docLine);
                md->setDocsForDefinition(!root->proto);
              }
#if 0
              if (md->briefDescription().isEmpty())
#endif
              {
                md->setBriefDescription(root->brief,root->briefFile,root->briefLine);
              }
              if (md->inbodyDocumentation().isEmpty())
              {
                md->setInbodyDocumentation(root->inbodyDocs,root->inbodyFile,root->inbodyLine);
              }
              md->setBodySegment(root->startLine,root->bodyLine,root->endBodyLine);
              md->setBodyDef(root->fileDef());
              md->addSectionsToDefinition(root->anchors);
              md->setRefItems(root->sli);
              md->setLanguage(root->lang);
              if (root->mGrpId!=-1) md->setMemberGroupId(root->mGrpId);
              addMemberToGroups(root,md.get());
            }
          }
        }
        //warn("define %s found in the following files:\n",root->name.data());
        //warn("Cannot determine where to add the documentation found "
        //     "at line %d of file %s. \n",
        //     root->startLine,root->fileName.data());
      }
    }
    else if (!root->doc.isEmpty() || !root->brief.isEmpty()) // define not found
    {
      static bool preEnabled = Config_getBool(ENABLE_PREPROCESSING);
      if (preEnabled)
      {
        warn(root->fileName,root->startLine,
             "documentation for unknown define %s found.\n",
             root->name.data()
            );
      }
      else
      {
        warn(root->fileName,root->startLine,
             "found documented #define %s but ignoring it because "
             "ENABLE_PREPROCESSING is NO.\n",
             root->name.data()
            );
      }
    }
  }
  for (const auto &e : root->children()) findDefineDocumentation(e.get());
}

//----------------------------------------------------------------------------

static void findDirDocumentation(const Entry *root)
{
  if (root->section == Entry::DIRDOC_SEC)
  {
    QCString normalizedName = root->name;
    normalizedName = substitute(normalizedName,"\\","/");
    //printf("root->docFile=%s normalizedName=%s\n",
    //    root->docFile.data(),normalizedName.data());
    if (root->docFile==normalizedName) // current dir?
    {
      int lastSlashPos=normalizedName.findRev('/');
      if (lastSlashPos!=-1) // strip file name
      {
        normalizedName=normalizedName.left(lastSlashPos);
      }
    }
    if (normalizedName.at(normalizedName.length()-1)!='/')
    {
      normalizedName+='/';
    }
    DirDef *dir,*matchingDir=0;
    SDict<DirDef>::Iterator sdi(*Doxygen::directories);
    for (sdi.toFirst();(dir=sdi.current());++sdi)
    {
      //printf("Dir: %s<->%s\n",dir->name().data(),normalizedName.data());
      if (dir->name().right(normalizedName.length())==normalizedName)
      {
        if (matchingDir)
        {
           warn(root->fileName,root->startLine,
             "\\dir command matches multiple directories.\n"
             "  Applying the command for directory %s\n"
             "  Ignoring the command for directory %s\n",
             matchingDir->name().data(),dir->name().data()
           );
        }
        else
        {
          matchingDir=dir;
        }
      }
    }
    if (matchingDir)
    {
      //printf("Match for with dir %s\n",matchingDir->name().data());
      matchingDir->setBriefDescription(root->brief,root->briefFile,root->briefLine);
      matchingDir->setDocumentation(root->doc,root->docFile,root->docLine);
      matchingDir->setRefItems(root->sli);
      addDirToGroups(root,matchingDir);
    }
    else
    {
      warn(root->fileName,root->startLine,"No matching "
          "directory found for command \\dir %s\n",normalizedName.data());
    }
  }
  for (const auto &e : root->children()) findDirDocumentation(e.get());
}


//----------------------------------------------------------------------------
// create a (sorted) list of separate documentation pages

static void buildPageList(Entry *root)
{
  if (root->section == Entry::PAGEDOC_SEC)
  {
    if (!root->name.isEmpty())
    {
      addRelatedPage(root);
    }
  }
  else if (root->section == Entry::MAINPAGEDOC_SEC)
  {
    QCString title=root->args.stripWhiteSpace();
    if (title.isEmpty()) title=theTranslator->trMainPage();
    //QCString name = Config_getBool(GENERATE_TREEVIEW)?"main":"index";
    QCString name = "index";
    addRefItem(root->sli,
               name,
               "page",
               name,
               title,
               0,0
               );
  }
  for (const auto &e : root->children()) buildPageList(e.get());
}

// search for the main page defined in this project
static void findMainPage(Entry *root)
{
  if (root->section == Entry::MAINPAGEDOC_SEC)
  {
    if (Doxygen::mainPage==0 && root->tagInfo()==0)
    {
      //printf("mainpage: docLine=%d startLine=%d\n",root->docLine,root->startLine);
      //printf("Found main page! \n======\n%s\n=======\n",root->doc.data());
      QCString title=root->args.stripWhiteSpace();
      //QCString indexName=Config_getBool(GENERATE_TREEVIEW)?"main":"index";
      QCString indexName="index";
      Doxygen::mainPage = createPageDef(root->docFile,root->docLine,
                              indexName, root->brief+root->doc+root->inbodyDocs,title);
      //setFileNameForSections(root->anchors,"index",Doxygen::mainPage);
      Doxygen::mainPage->setBriefDescription(root->brief,root->briefFile,root->briefLine);
      Doxygen::mainPage->setBodySegment(root->startLine,root->startLine,-1);
      Doxygen::mainPage->setFileName(indexName);
      Doxygen::mainPage->setLocalToc(root->localToc);
      addPageToContext(Doxygen::mainPage,root);

      const SectionInfo *si = SectionManager::instance().find(Doxygen::mainPage->name());
      if (si)
      {
        if (si->lineNr() != -1)
        {
          warn(root->fileName,root->startLine,"multiple use of section label '%s' for main page, (first occurrence: %s, line %d)",
               Doxygen::mainPage->name().data(),si->fileName().data(),si->lineNr());
        }
        else
        {
          warn(root->fileName,root->startLine,"multiple use of section label '%s' for main page, (first occurrence: %s)",
               Doxygen::mainPage->name().data(),si->fileName().data());
        }
      }
      else
      {
        // a page name is a label as well! but should no be double either
        SectionManager::instance().add(
          Doxygen::mainPage->name(),
          indexName,
          root->startLine,
          Doxygen::mainPage->title(),
          SectionType::Page,
          0); // level 0
      }
      Doxygen::mainPage->addSectionsToDefinition(root->anchors);
    }
    else if (root->tagInfo()==0)
    {
      warn(root->fileName,root->startLine,
           "found more than one \\mainpage comment block! (first occurrence: %s, line %d), Skipping current block!",
           Doxygen::mainPage->docFile().data(),Doxygen::mainPage->getStartBodyLine());
    }
  }
  for (const auto &e : root->children()) findMainPage(e.get());
}

// search for the main page imported via tag files and add only the section labels
static void findMainPageTagFiles(Entry *root)
{
  if (root->section == Entry::MAINPAGEDOC_SEC)
  {
    if (Doxygen::mainPage && root->tagInfo())
    {
      Doxygen::mainPage->addSectionsToDefinition(root->anchors);
    }
  }
  for (const auto &e : root->children()) findMainPageTagFiles(e.get());
}

static void computePageRelations(Entry *root)
{
  if ((root->section==Entry::PAGEDOC_SEC ||
       root->section==Entry::MAINPAGEDOC_SEC
      )
      && !root->name.isEmpty()
     )
  {
    PageDef *pd = root->section==Entry::PAGEDOC_SEC ?
                    Doxygen::pageSDict->find(root->name) :
                    Doxygen::mainPage;
    if (pd)
    {
      for (const BaseInfo &bi : root->extends)
      {
        PageDef *subPd = Doxygen::pageSDict->find(bi.name);
        if (pd==subPd)
        {
         term("page defined at line %d of file %s with label %s is a direct "
             "subpage of itself! Please remove this cyclic dependency.\n",
              pd->docLine(),pd->docFile().data(),pd->name().data());
        }
        else if (subPd)
        {
          pd->addInnerCompound(subPd);
          //printf("*** Added subpage relation: %s->%s\n",
          //    pd->name().data(),subPd->name().data());
        }
      }
    }
  }
  for (const auto &e : root->children()) computePageRelations(e.get());
}

static void checkPageRelations()
{
  PageSDict::Iterator pdi(*Doxygen::pageSDict);
  PageDef *pd=0;
  for (pdi.toFirst();(pd=pdi.current());++pdi)
  {
    Definition *ppd = pd->getOuterScope();
    while (ppd)
    {
      if (ppd==pd)
      {
        term("page defined at line %d of file %s with label %s is a subpage "
            "of itself! Please remove this cyclic dependency.\n",
            pd->docLine(),pd->docFile().data(),pd->name().data());
      }
      ppd=ppd->getOuterScope();
    }
  }
}

//----------------------------------------------------------------------------

static void resolveUserReferences()
{
  for (const auto &si : SectionManager::instance())
  {
    //printf("si->label='%s' si->definition=%s si->fileName='%s'\n",
    //        si->label.data(),si->definition?si->definition->name().data():"<none>",
    //        si->fileName.data());
    PageDef *pd=0;

    // hack: the items of a todo/test/bug/deprecated list are all fragments from
    // different files, so the resulting section's all have the wrong file
    // name (not from the todo/test/bug/deprecated list, but from the file in
    // which they are defined). We correct this here by looking at the
    // generated section labels!
    for (const RefListManager::Ptr &rl : RefListManager::instance())
    {
      QCString label="_"+rl->listName(); // "_todo", "_test", ...
      if (si->label().left(label.length())==label)
      {
        si->setFileName(rl->listName());
        si->setGenerated(TRUE);
        break;
      }
    }

    //printf("start: si->label=%s si->fileName=%s\n",si->label.data(),si->fileName.data());
    if (!si->generated())
    {
      // if this section is in a page and the page is in a group, then we
      // have to adjust the link file name to point to the group.
      if (!si->fileName().isEmpty() &&
          (pd=Doxygen::pageSDict->find(si->fileName())) &&
          pd->getGroupDef())
      {
        si->setFileName(pd->getGroupDef()->getOutputFileBase());
      }

      if (si->definition())
      {
        // TODO: there should be one function in Definition that returns
        // the file to link to, so we can avoid the following tests.
        const GroupDef *gd=0;
        if (si->definition()->definitionType()==Definition::TypeMember)
        {
          gd = (dynamic_cast<MemberDef *>(si->definition()))->getGroupDef();
        }

        if (gd)
        {
          si->setFileName(gd->getOutputFileBase());
        }
        else
        {
          //si->fileName=si->definition->getOutputFileBase().copy();
          //printf("Setting si->fileName to %s\n",si->fileName.data());
        }
      }
    }
    //printf("end: si->label=%s si->fileName=%s\n",si->label.data(),si->fileName.data());
  }
}



//----------------------------------------------------------------------------
// generate all separate documentation pages


static void generatePageDocs()
{
  //printf("documentedPages=%d real=%d\n",documentedPages,Doxygen::pageSDict->count());
  if (documentedPages==0) return;
  PageSDict::Iterator pdi(*Doxygen::pageSDict);
  PageDef *pd=0;
  for (pdi.toFirst();(pd=pdi.current());++pdi)
  {
    if (!pd->getGroupDef() && !pd->isReference())
    {
      msg("Generating docs for page %s...\n",pd->name().data());
      Doxygen::insideMainPage=TRUE;
      pd->writeDocumentation(*g_outputList);
      Doxygen::insideMainPage=FALSE;
    }
  }
}

//----------------------------------------------------------------------------
// create a (sorted) list & dictionary of example pages

static void buildExampleList(Entry *root)
{
  if ((root->section==Entry::EXAMPLE_SEC || root->section==Entry::EXAMPLE_LINENO_SEC) && !root->name.isEmpty())
  {
    if (Doxygen::exampleSDict->find(root->name))
    {
      warn(root->fileName,root->startLine,
          "Example %s was already documented. Ignoring "
          "documentation found here.",
          root->name.data()
          );
    }
    else
    {
      PageDef *pd=createPageDef(root->fileName,root->startLine,
          root->name,root->brief+root->doc+root->inbodyDocs,root->args);
      pd->setBriefDescription(root->brief,root->briefFile,root->briefLine);
      pd->setFileName(convertNameToFile(pd->name()+"-example",FALSE,TRUE));
      pd->addSectionsToDefinition(root->anchors);
      pd->setLanguage(root->lang);
      pd->setShowLineNo(root->section==Entry::EXAMPLE_LINENO_SEC);

      Doxygen::exampleSDict->inSort(root->name,pd);
      //we don't add example to groups
      //addExampleToGroups(root,pd);
    }
  }
  for (const auto &e : root->children()) buildExampleList(e.get());
}

//----------------------------------------------------------------------------
// prints the Entry tree (for debugging)

void printNavTree(Entry *root,int indent)
{
  QCString indentStr;
  indentStr.fill(' ',indent);
  msg("%s%s (sec=0x%x)\n",
      indentStr.isEmpty()?"":indentStr.data(),
      root->name.isEmpty()?"<empty>":root->name.data(),
      root->section);
  for (const auto &e : root->children())
  {
    printNavTree(e.get(),indent+2);
  }
}


//----------------------------------------------------------------------------
// generate the example documentation

static void generateExampleDocs()
{
  g_outputList->disable(OutputGenerator::Man);
  PageSDict::Iterator pdi(*Doxygen::exampleSDict);
  PageDef *pd=0;
  for (pdi.toFirst();(pd=pdi.current());++pdi)
  {
    msg("Generating docs for example %s...\n",pd->name().data());
    auto intf = Doxygen::parserManager->getCodeParser(".c"); // TODO: do this on code type
    intf->resetCodeParserState();
    QCString n=pd->getOutputFileBase();
    startFile(*g_outputList,n,n,pd->name());
    startTitle(*g_outputList,n);
    g_outputList->docify(pd->name());
    endTitle(*g_outputList,n,0);
    g_outputList->startContents();
    QCString lineNoOptStr;
    if (pd->showLineNo())
    {
      lineNoOptStr="{lineno}";
    }
    g_outputList->generateDoc(pd->docFile(),                            // file
                         pd->docLine(),                            // startLine
                         pd,                                       // context
                         0,                                        // memberDef
                         pd->documentation()+"\n\n\\include"+lineNoOptStr+" "+pd->name(), // docs
                         TRUE,                                     // index words
                         TRUE,                                     // is example
                         pd->name(),
                         FALSE,
                         FALSE,
                         Config_getBool(MARKDOWN_SUPPORT)
                        );
    endFile(*g_outputList); // contains g_outputList->endContents()
  }
  g_outputList->enable(OutputGenerator::Man);
}

//----------------------------------------------------------------------------
// generate module pages

static void generateGroupDocs()
{
  GroupSDict::Iterator gli(*Doxygen::groupSDict);
  GroupDef *gd;
  for (gli.toFirst();(gd=gli.current());++gli)
  {
    if (!gd->isReference())
    {
      gd->writeDocumentation(*g_outputList);
    }
  }
}

//----------------------------------------------------------------------------

//static void generatePackageDocs()
//{
//  writePackageIndex(*g_outputList);
//
//  if (Doxygen::packageDict.count()>0)
//  {
//    PackageSDict::Iterator pdi(Doxygen::packageDict);
//    PackageDef *pd;
//    for (pdi.toFirst();(pd=pdi.current());++pdi)
//    {
//      pd->writeDocumentation(*g_outputList);
//    }
//  }
//}

//----------------------------------------------------------------------------
// generate module pages

static void generateNamespaceClassDocs(ClassSDict *d)
{
  // for each class in the namespace...
  ClassSDict::Iterator cli(*d);
  ClassDef *cd;
  for ( ; (cd=cli.current()) ; ++cli )
  {
    if ( ( cd->isLinkableInProject() &&
           cd->templateMaster()==0
         ) // skip external references, anonymous compounds and
           // template instances and nested classes
         && !cd->isHidden() && !cd->isEmbeddedInOuterScope()
       )
    {
      msg("Generating docs for compound %s...\n",cd->name().data());

      cd->writeDocumentation(*g_outputList);
      cd->writeMemberList(*g_outputList);
    }
    cd->writeDocumentationForInnerClasses(*g_outputList);
  }
}

static void generateNamespaceDocs()
{
  static bool sliceOpt = Config_getBool(OPTIMIZE_OUTPUT_SLICE);

  //writeNamespaceIndex(*g_outputList);

  NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
  NamespaceDef *nd;
  // for each namespace...
  for (;(nd=nli.current());++nli)
  {

    if (nd->isLinkableInProject())
    {
      msg("Generating docs for namespace %s\n",nd->name().data());
      nd->writeDocumentation(*g_outputList);
    }

    generateNamespaceClassDocs(nd->getClassSDict());
    if (sliceOpt)
    {
      generateNamespaceClassDocs(nd->getInterfaceSDict());
      generateNamespaceClassDocs(nd->getStructSDict());
      generateNamespaceClassDocs(nd->getExceptionSDict());
    }
  }
}

#if defined(_WIN32)
static QCString fixSlashes(QCString &s)
{
  QCString result;
  uint i;
  for (i=0;i<s.length();i++)
  {
    switch(s.at(i))
    {
      case '/':
      case '\\':
        result+="\\\\";
        break;
      default:
        result+=s.at(i);
    }
  }
  return result;
}
#endif


//----------------------------------------------------------------------------

/*! Generate a template version of the configuration file.
 *  If the \a shortList parameter is TRUE a configuration file without
 *  comments will be generated.
 */
static void generateConfigFile(const char *configFile,bool shortList,
                               bool updateOnly=FALSE)
{
  QFile f;
  bool fileOpened=openOutputFile(configFile,f);
  bool writeToStdout=(configFile[0]=='-' && configFile[1]=='\0');
  if (fileOpened)
  {
    FTextStream t(&f);
    Config::writeTemplate(t,shortList,updateOnly);
    if (!writeToStdout)
    {
      if (!updateOnly)
      {
        msg("\n\nConfiguration file '%s' created.\n\n",configFile);
        msg("Now edit the configuration file and enter\n\n");
        if (qstrcmp(configFile,"Doxyfile") || qstrcmp(configFile,"doxyfile"))
          msg("  doxygen %s\n\n",configFile);
        else
          msg("  doxygen\n\n");
        msg("to generate the documentation for your project\n\n");
      }
      else
      {
        msg("\n\nConfiguration file '%s' updated.\n\n",configFile);
      }
    }
  }
  else
  {
    term("Cannot open file %s for writing\n",configFile);
  }
}
static void compareDoxyfile()
{
  QFile f;
  char configFile[2];
  configFile[0] = '-';
  configFile[1] = '\0';
  bool fileOpened=openOutputFile(configFile,f);
  if (fileOpened)
  {
    FTextStream t(&f);
    Config::compareDoxyfile(t);
  }
  else
  {
    term("Cannot open file %s for writing\n",configFile);
  }
}
//----------------------------------------------------------------------------
// read and parse a tag file

//static bool readLineFromFile(QFile &f,QCString &s)
//{
//  char c=0;
//  s.resize(0);
//  while (!f.atEnd() && (c=f.getch())!='\n') s+=c;
//  return f.atEnd();
//}

//----------------------------------------------------------------------------

static void readTagFile(const std::shared_ptr<Entry> &root,const char *tl)
{
  QCString tagLine = tl;
  QCString fileName;
  QCString destName;
  int eqPos = tagLine.find('=');
  if (eqPos!=-1) // tag command contains a destination
  {
    fileName = tagLine.left(eqPos).stripWhiteSpace();
    destName = tagLine.right(tagLine.length()-eqPos-1).stripWhiteSpace();
    if (fileName.isEmpty() || destName.isEmpty()) return;
    QFileInfo fi(fileName);
    Doxygen::tagDestinationDict.insert(fi.absFilePath().utf8(),new QCString(destName));
    //printf("insert tagDestination %s->%s\n",fi.fileName().data(),destName.data());
  }
  else
  {
    fileName = tagLine;
  }

  QFileInfo fi(fileName);
  if (!fi.exists() || !fi.isFile())
  {
    err("Tag file '%s' does not exist or is not a file. Skipping it...\n",
        fileName.data());
    return;
  }

  if (!destName.isEmpty())
    msg("Reading tag file '%s', location '%s'...\n",fileName.data(),destName.data());
  else
    msg("Reading tag file '%s'...\n",fileName.data());

  parseTagFile(root,fi.absFilePath().utf8());
}

//----------------------------------------------------------------------------
static void copyLatexStyleSheet()
{
  const StringVector &latexExtraStyleSheet = Config_getList(LATEX_EXTRA_STYLESHEET);
  for (const auto &sheet : latexExtraStyleSheet)
  {
    QCString fileName = sheet.c_str();
    if (!fileName.isEmpty())
    {
      QFileInfo fi(fileName);
      if (!fi.exists())
      {
        err("Style sheet '%s' specified by LATEX_EXTRA_STYLESHEET does not exist!\n",fileName.data());
      }
      else
      {
        QCString destFileName = Config_getString(LATEX_OUTPUT)+"/"+fi.fileName().data();
        if (!checkExtension(fi.fileName().data(), LATEX_STYLE_EXTENSION))
        {
          destFileName += LATEX_STYLE_EXTENSION;
        }
        copyFile(fileName, destFileName);
      }
    }
  }
}

//----------------------------------------------------------------------------
static void copyStyleSheet()
{
  QCString htmlStyleSheet = Config_getString(HTML_STYLESHEET);
  if (!htmlStyleSheet.isEmpty())
  {
    QFileInfo fi(htmlStyleSheet);
    if (!fi.exists())
    {
      err("Style sheet '%s' specified by HTML_STYLESHEET does not exist!\n",htmlStyleSheet.data());
      htmlStyleSheet = Config_updateString(HTML_STYLESHEET,""); // revert to the default
    }
    else
    {
      QCString destFileName = Config_getString(HTML_OUTPUT)+"/"+fi.fileName().data();
      copyFile(htmlStyleSheet,destFileName);
    }
  }
  const StringVector &htmlExtraStyleSheet = Config_getList(HTML_EXTRA_STYLESHEET);
  for (const auto &sheet : htmlExtraStyleSheet)
  {
    QCString fileName = sheet.c_str();
    if (!fileName.isEmpty())
    {
      QFileInfo fi(fileName);
      if (!fi.exists())
      {
        err("Style sheet '%s' specified by HTML_EXTRA_STYLESHEET does not exist!\n",fileName.data());
      }
      else if (fi.fileName()=="doxygen.css" || fi.fileName()=="tabs.css" || fi.fileName()=="navtree.css")
      {
        err("Style sheet %s specified by HTML_EXTRA_STYLESHEET is already a built-in stylesheet. Please use a different name\n",fi.fileName().data());
      }
      else
      {
        QCString destFileName = Config_getString(HTML_OUTPUT)+"/"+fi.fileName().data();
        copyFile(fileName, destFileName);
      }
    }
  }
}

static void copyLogo(const QCString &outputOption)
{
  QCString projectLogo = Config_getString(PROJECT_LOGO);
  if (!projectLogo.isEmpty())
  {
    QFileInfo fi(projectLogo);
    if (!fi.exists())
    {
      err("Project logo '%s' specified by PROJECT_LOGO does not exist!\n",projectLogo.data());
      projectLogo = Config_updateString(PROJECT_LOGO,""); // revert to the default
    }
    else
    {
      QCString destFileName = outputOption+"/"+fi.fileName().data();
      copyFile(projectLogo,destFileName);
      Doxygen::indexList->addImageFile(fi.fileName().data());
    }
  }
}

static void copyExtraFiles(const StringVector &files,const QCString &filesOption,const QCString &outputOption)
{
  for (const auto &file : files)
  {
    QCString fileName = file.c_str();
    if (!fileName.isEmpty())
    {
      QFileInfo fi(fileName);
      if (!fi.exists())
      {
        err("Extra file '%s' specified in %s does not exist!\n", fileName.data(),filesOption.data());
      }
      else
      {
        QCString destFileName = outputOption+"/"+fi.fileName().data();
        Doxygen::indexList->addImageFile(fi.fileName().utf8());
        copyFile(fileName, destFileName);
      }
    }
  }
}

//----------------------------------------------------------------------------

static void generateDiskNames()
{
  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    struct FileEntry
    {
      FileEntry(const QCString &p,FileDef *fd) : path(p), fileDef(fd) {}
      QCString path;
      FileDef *fileDef;
    };

    // collect the entry for which to compute the longest common prefix (LCP) of the path
    std::vector<FileEntry> fileEntries;
    for (const auto &fd : *fn)
    {
      if (!fd->isReference()) // skip external references
      {
        fileEntries.emplace_back(fd->getPath(),fd.get());
      }
    }

    size_t size = fileEntries.size();

    if (size==1) // name if unique, so diskname is simply the name
    {
      FileDef *fd = fileEntries[0].fileDef;
      fd->setDiskName(fn->fileName());
    }
    else if (size>1) // multiple occurrences of the same file name
    {
      // sort the array
      std::sort(fileEntries.begin(),
                fileEntries.end(),
                [](const FileEntry &fe1,const FileEntry &fe2)
                { return qstrcmp(fe1.path.data(),fe2.path.data())<0; }
               );

      // since the entries are sorted, the common prefix of the whole array is same
      // as the common prefix between the first and last entry
      const FileEntry &first = fileEntries[0];
      const FileEntry &last =  fileEntries[size-1];
      int first_path_size = static_cast<int>(first.path.size())-1; // -1 to skip trailing slash
      int last_path_size  = static_cast<int>(last.path.size())-1;  // -1 to skip trailing slash
      int j=0;
      for (int i=0;i<first_path_size && i<last_path_size;i++)
      {
        if (first.path[i]=='/') j=i;
        if (first.path[i]!=last.path[i]) break;
      }

      // add non-common part of the path to the name
      for (auto &fileEntry : fileEntries)
      {
         QCString prefix = fileEntry.path.right(fileEntry.path.length()-j-1);
         fileEntry.fileDef->setName(prefix+fn->fileName());
         //printf("!!!!!!!! non unique disk name=%s:%s\n",prefix.data(),fn->fileName());
         fileEntry.fileDef->setDiskName(prefix+fn->fileName());
      }
    }
  }
}



//----------------------------------------------------------------------------

static std::unique_ptr<OutlineParserInterface> getParserForFile(const char *fn)
{
  QCString fileName=fn;
  QCString extension;
  int sep = fileName.findRev('/');
  int ei = fileName.findRev('.');
  if (ei!=-1 && (sep==-1 || ei>sep)) // matches dir/file.ext but not dir.1/file
  {
    extension=fileName.right(fileName.length()-ei);
  }
  else
  {
    extension = ".no_extension";
  }

  return Doxygen::parserManager->getOutlineParser(extension);
}

static std::shared_ptr<Entry> parseFile(OutlineParserInterface &parser,
                      FileDef *fd,const char *fn,
                      ClangTUParser *clangParser,bool newTU)
{
  QCString fileName=fn;
  QCString extension;
  int ei = fileName.findRev('.');
  if (ei!=-1)
  {
    extension=fileName.right(fileName.length()-ei);
  }
  else
  {
    extension = ".no_extension";
  }

  QFileInfo fi(fileName);
  BufStr preBuf(fi.size()+4096);

  if (Config_getBool(ENABLE_PREPROCESSING) &&
      parser.needsPreprocessing(extension))
  {
    Preprocessor preprocessor;
    const StringVector &includePath = Config_getList(INCLUDE_PATH);
    for (const auto &s : includePath)
    {
      preprocessor.addSearchDir(QFileInfo(s.c_str()).absFilePath().utf8());
    }
    BufStr inBuf(fi.size()+4096);
    msg("Preprocessing %s...\n",fn);
    readInputFile(fileName,inBuf);
    preprocessor.processFile(fileName,inBuf,preBuf);
  }
  else // no preprocessing
  {
    msg("Reading %s...\n",fn);
    readInputFile(fileName,preBuf);
  }
  if (preBuf.data() && preBuf.curPos()>0 && *(preBuf.data()+preBuf.curPos()-1)!='\n')
  {
    preBuf.addChar('\n'); // add extra newline to help parser
  }

  BufStr convBuf(preBuf.curPos()+1024);

  // convert multi-line C++ comments to C style comments
  convertCppComments(&preBuf,&convBuf,fileName);

  convBuf.addChar('\0');

  std::shared_ptr<Entry> fileRoot = std::make_shared<Entry>();
  // use language parse to parse the file
  if (clangParser)
  {
    if (newTU) clangParser->parse();
    clangParser->switchToFile(fd);
  }
  parser.parseInput(fileName,convBuf.data(),fileRoot,clangParser);
  fileRoot->setFileDef(fd);
  return fileRoot;
}

//! parse the list of input files
static void parseFilesMultiThreading(const std::shared_ptr<Entry> &root)
{
#if USE_LIBCLANG
  if (Doxygen::clangAssistedParsing)
  {
    StringUnorderedSet processedFiles;

    // create a dictionary with files to process
    StringUnorderedSet filesToProcess;
    for (const auto &s : g_inputFiles)
    {
      filesToProcess.insert(s);
    }

    std::mutex processedFilesLock;
    // process source files (and their include dependencies)
    std::size_t numThreads = static_cast<std::size_t>(Config_getInt(NUM_PROC_THREADS));
    if (numThreads==0)
    {
      numThreads = std::thread::hardware_concurrency();
    }
    msg("Processing input using %zu threads.\n",numThreads);
    ThreadPool threadPool(numThreads);
    using FutureType = std::vector< std::shared_ptr<Entry> >;
    std::vector< std::future< FutureType > > results;
    for (const auto &s : g_inputFiles)
    {
      bool ambig;
      FileDef *fd=findFileDef(Doxygen::inputNameLinkedMap,s.c_str(),ambig);
      ASSERT(fd!=0);
      if (fd->isSource() && !fd->isReference()) // this is a source file
      {
        // lambda representing the work to executed by a thread
        auto processFile = [s,&filesToProcess,&processedFilesLock,&processedFiles]() {
          bool ambig_l;
          std::vector< std::shared_ptr<Entry> > roots;
          FileDef *fd_l = findFileDef(Doxygen::inputNameLinkedMap,s.c_str(),ambig_l);
          auto clangParser = ClangParser::instance()->createTUParser(fd_l);
          auto parser = getParserForFile(s.c_str());
          auto fileRoot { parseFile(*parser.get(),fd_l,s.c_str(),clangParser.get(),true) };
          roots.push_back(fileRoot);

          // Now process any include files in the same translation unit
          // first. When libclang is used this is much more efficient.
          for (auto incFile : clangParser->filesInSameTU())
          {
            if (filesToProcess.find(incFile)!=filesToProcess.end())
            {
              bool needsToBeProcessed;
              {
                std::lock_guard<std::mutex> lock(processedFilesLock);
                needsToBeProcessed = processedFiles.find(incFile)==processedFiles.end();
                if (needsToBeProcessed) processedFiles.insert(incFile);
              }
              if (incFile!=s && needsToBeProcessed)
              {
                FileDef *ifd=findFileDef(Doxygen::inputNameLinkedMap,incFile.c_str(),ambig_l);
                if (ifd && !ifd->isReference())
                {
                  //printf("  Processing %s in same translation unit as %s\n",incFile,s->c_str());
                  fileRoot = parseFile(*parser.get(),ifd,incFile.c_str(),clangParser.get(),false);
                  roots.push_back(fileRoot);
                }
              }
            }
          }
          return roots;
        };
        // dispatch the work and collect the future results
        results.emplace_back(threadPool.queue(processFile));
      }
    }
    // synchronise with the Entry result lists produced and add them to the root
    for (auto &f : results)
    {
      auto l = f.get();
      for (auto &e : l)
      {
        root->moveToSubEntryAndKeep(e);
      }
    }
    // process remaining files
    results.clear();
    for (const auto &s : g_inputFiles)
    {
      if (processedFiles.find(s)==processedFiles.end()) // not yet processed
      {
        // lambda representing the work to executed by a thread
        auto processFile = [s]() {
          bool ambig;
          std::vector< std::shared_ptr<Entry> > roots;
          FileDef *fd=findFileDef(Doxygen::inputNameLinkedMap,s.c_str(),ambig);
          auto clangParser = ClangParser::instance()->createTUParser(fd);
          auto parser { getParserForFile(s.c_str()) };
          auto fileRoot = parseFile(*parser.get(),fd,s.c_str(),clangParser.get(),true);
          roots.push_back(fileRoot);
          return roots;
        };
        // dispatch the work and collect the future results
        results.emplace_back(threadPool.queue(processFile));
      }
    }
    // synchronise with the Entry result lists produced and add them to the root
    for (auto &f : results)
    {
      auto l = f.get();
      for (auto &e : l)
      {
        root->moveToSubEntryAndKeep(e);
      }
    }
  }
  else // normal processing
#endif
  {
    std::size_t numThreads = std::thread::hardware_concurrency();
    msg("Processing input using %zu threads.\n",numThreads);
    ThreadPool threadPool(numThreads);
    using FutureType = std::shared_ptr<Entry>;
    std::vector< std::future< FutureType > > results;
    for (const auto &s : g_inputFiles)
    {
      // lambda representing the work to executed by a thread
      auto processFile = [s]() {
        bool ambig;
        FileDef *fd=findFileDef(Doxygen::inputNameLinkedMap,s.c_str(),ambig);
        auto parser = getParserForFile(s.c_str());
        auto fileRoot = parseFile(*parser.get(),fd,s.c_str(),nullptr,true);
        return fileRoot;
      };
      // dispatch the work and collect the future results
      results.emplace_back(threadPool.queue(processFile));
    }
    // synchronise with the Entry results produced and add them to the root
    for (auto &f : results)
    {
      root->moveToSubEntryAndKeep(f.get());
    }
  }
}

//! parse the list of input files
static void parseFilesSingleThreading(const std::shared_ptr<Entry> &root)
{
#if USE_LIBCLANG
  if (Doxygen::clangAssistedParsing)
  {
    StringUnorderedSet processedFiles;

    // create a dictionary with files to process
    StringUnorderedSet filesToProcess;
    for (const auto &s : g_inputFiles)
    {
      filesToProcess.insert(s);
    }

    // process source files (and their include dependencies)
    for (const auto &s : g_inputFiles)
    {
      bool ambig;
      FileDef *fd=findFileDef(Doxygen::inputNameLinkedMap,s.c_str(),ambig);
      ASSERT(fd!=0);
      if (fd->isSource() && !fd->isReference()) // this is a source file
      {
        auto clangParser = ClangParser::instance()->createTUParser(fd);
        auto parser { getParserForFile(s.c_str()) };
        auto fileRoot = parseFile(*parser.get(),fd,s.c_str(),clangParser.get(),true);
        root->moveToSubEntryAndKeep(fileRoot);
        processedFiles.insert(s);

        // Now process any include files in the same translation unit
        // first. When libclang is used this is much more efficient.
        for (auto incFile : clangParser->filesInSameTU())
        {
          //printf("    file %s\n",incFile.c_str());
          if (filesToProcess.find(incFile)!=filesToProcess.end() && // file need to be processed
              processedFiles.find(incFile)==processedFiles.end())   // and is not processed already
          {
            FileDef *ifd=findFileDef(Doxygen::inputNameLinkedMap,incFile.c_str(),ambig);
            if (ifd && !ifd->isReference())
            {
              //printf("  Processing %s in same translation unit as %s\n",incFile.c_str(),s.c_str());
              fileRoot = parseFile(*parser.get(),ifd,incFile.c_str(),clangParser.get(),false);
              root->moveToSubEntryAndKeep(fileRoot);
              processedFiles.insert(incFile);
            }
          }
        }
      }
    }
    // process remaining files
    for (const auto &s : g_inputFiles)
    {
      if (processedFiles.find(s)==processedFiles.end()) // not yet processed
      {
        bool ambig;
        FileDef *fd=findFileDef(Doxygen::inputNameLinkedMap,s.c_str(),ambig);
        auto clangParser = ClangParser::instance()->createTUParser(fd);
        auto parser { getParserForFile(s.c_str()) };
        auto fileRoot = parseFile(*parser.get(),fd,s.c_str(),clangParser.get(),true);
        root->moveToSubEntryAndKeep(fileRoot);
        processedFiles.insert(s);
      }
    }
  }
  else // normal processing
#endif
  {
    for (const auto &s : g_inputFiles)
    {
      bool ambig;
      FileDef *fd=findFileDef(Doxygen::inputNameLinkedMap,s.c_str(),ambig);
      ASSERT(fd!=0);
      std::unique_ptr<OutlineParserInterface> parser { getParserForFile(s.c_str()) };
      std::shared_ptr<Entry> fileRoot = parseFile(*parser.get(),fd,s.c_str(),nullptr,true);
      root->moveToSubEntryAndKeep(fileRoot);
    }
  }
}

// resolves a path that may include symlinks, if a recursive symlink is
// found an empty string is returned.
static QCString resolveSymlink(QCString path)
{
  int sepPos=0;
  int oldPos=0;
  QFileInfo fi;
  QDict<void> nonSymlinks;
  QDict<void> known;
  QCString result = path;
  QCString oldPrefix = "/";
  do
  {
#ifdef WIN32
    // UNC path, skip server and share name
    if (sepPos==0 && (result.left(2)=="//" || result.left(2)=="\\\\"))
      sepPos = result.find('/',2);
    if (sepPos!=-1)
      sepPos = result.find('/',sepPos+1);
#else
    sepPos = result.find('/',sepPos+1);
#endif
    QCString prefix = sepPos==-1 ? result : result.left(sepPos);
    if (nonSymlinks.find(prefix)==0)
    {
      fi.setFile(prefix);
      if (fi.isSymLink())
      {
        QString target = fi.readLink();
        bool isRelative = QFileInfo(target).isRelative();
        if (isRelative)
        {
          target = QDir::cleanDirPath(oldPrefix+"/"+target.data());
        }
        if (sepPos!=-1)
        {
          if (fi.isDir() && target.length()>0 && target.at(target.length()-1)!='/')
          {
            target+='/';
          }
          target+=result.mid(sepPos);
        }
        result = QDir::cleanDirPath(target).data();
        sepPos = 0;
        if (known.find(result)) return QCString(); // recursive symlink!
        known.insert(result,(void*)0x8);
        if (isRelative)
        {
          sepPos = oldPos;
        }
        else // link to absolute path
        {
          sepPos = 0;
          oldPrefix = "/";
        }
      }
      else
      {
        nonSymlinks.insert(prefix,(void*)0x8);
        oldPrefix = prefix;
      }
      oldPos = sepPos;
    }
  }
  while (sepPos!=-1);
  return QDir::cleanDirPath(result).data();
}

static std::mutex g_pathsVisitedMutex;
static StringUnorderedSet g_pathsVisited(1009);

//----------------------------------------------------------------------------
// Read all files matching at least one pattern in 'patList' in the
// directory represented by 'fi'.
// The directory is read iff the recursiveFlag is set.
// The contents of all files is append to the input string

static int readDir(QFileInfo *fi,
            FileNameLinkedMap *fnMap,
            StringUnorderedSet *exclSet,
            const StringVector *patList,
            const StringVector *exclPatList,
            StringVector *resultList,
            StringUnorderedSet *resultSet,
            bool errorIfNotExist,
            bool recursive,
            StringUnorderedSet *killSet,
            StringSet *paths
           )
{
  QCString dirName = fi->absFilePath().utf8();
  if (paths && !dirName.isEmpty())
  {
    paths->insert(dirName.data());
  }
  if (fi->isSymLink())
  {
    dirName = resolveSymlink(dirName.data());
    if (dirName.isEmpty()) return 0;            // recursive symlink

    std::lock_guard<std::mutex> lock(g_pathsVisitedMutex);
    if (g_pathsVisited.find(dirName.str())!=g_pathsVisited.end()) return 0; // already visited path
    g_pathsVisited.insert(dirName.str());
  }
  QDir dir(dirName);
  dir.setFilter( QDir::Files | QDir::Dirs | QDir::Hidden );
  int totalSize=0;
  msg("Searching for files in directory %s\n", fi->absFilePath().data());
  //printf("killSet=%p count=%d\n",killSet,killSet ? (int)killSet->count() : -1);

  const QFileInfoList *list = dir.entryInfoList();
  if (list)
  {
    QFileInfoListIterator it( *list );
    QFileInfo *cfi;

    while ((cfi=it.current()))
    {
      if (exclSet==0 || exclSet->find(cfi->absFilePath().utf8().data())==exclSet->end())
      { // file should not be excluded
        //printf("killSet->find(%s)\n",cfi->absFilePath().data());
        if (!cfi->exists() || !cfi->isReadable())
        {
          if (errorIfNotExist)
          {
            warn_uncond("source '%s' is not a readable file or directory... skipping.\n",cfi->absFilePath().data());
          }
        }
        else if (cfi->isFile() &&
            (!Config_getBool(EXCLUDE_SYMLINKS) || !cfi->isSymLink()) &&
            (patList==0 || patternMatch(*cfi,*patList)) &&
            (exclPatList==0 || !patternMatch(*cfi,*exclPatList)) &&
            (killSet==0 || killSet->find(cfi->absFilePath().utf8().data())==killSet->end())
            )
        {
          totalSize+=cfi->size()+cfi->absFilePath().length()+4;
          QCString name=cfi->fileName().utf8();
          //printf("New file %s\n",name.data());
          if (fnMap)
          {
            std::unique_ptr<FileDef> fd { createFileDef(cfi->dirPath().utf8()+"/",name) };
            FileName *fn=0;
            if (!name.isEmpty())
            {
              fn = fnMap->add(name,cfi->absFilePath().utf8());
              fn->push_back(std::move(fd));
            }
          }
          if (resultList) resultList->push_back(cfi->absFilePath().utf8().data());
          if (resultSet) resultSet->insert(cfi->absFilePath().utf8().data());
          if (killSet) killSet->insert(cfi->absFilePath().utf8().data());
        }
        else if (recursive &&
            (!Config_getBool(EXCLUDE_SYMLINKS) || !cfi->isSymLink()) &&
            cfi->isDir() &&
            (exclPatList==0 || !patternMatch(*cfi,*exclPatList)) &&
            cfi->fileName().at(0)!='.') // skip "." ".." and ".dir"
        {
          cfi->setFile(cfi->absFilePath());
          totalSize+=readDir(cfi,fnMap,exclSet,
              patList,exclPatList,resultList,resultSet,errorIfNotExist,
              recursive,killSet,paths);
        }
      }
      ++it;
    }
  }
  return totalSize;
}


//----------------------------------------------------------------------------
// read a file or all files in a directory and append their contents to the
// input string. The names of the files are appended to the 'fiList' list.

int readFileOrDirectory(const char *s,
                        FileNameLinkedMap *fnMap,
                        StringUnorderedSet *exclSet,
                        const StringVector *patList,
                        const StringVector *exclPatList,
                        StringVector *resultList,
                        StringUnorderedSet *resultSet,
                        bool recursive,
                        bool errorIfNotExist,
                        StringUnorderedSet *killSet,
                        StringSet *paths
                       )
{
  //printf("killSet count=%d\n",killSet ? (int)killSet->size() : -1);
  // strip trailing slashes
  if (s==0) return 0;
  QCString fs = s;
  char lc = fs.at(fs.length()-1);
  if (lc=='/' || lc=='\\') fs = fs.left(fs.length()-1);

  QFileInfo fi(fs);
  //printf("readFileOrDirectory(%s)\n",s);
  int totalSize=0;
  {
    if (exclSet==0 || exclSet->find(fi.absFilePath().utf8().data())==exclSet->end())
    {
      if (!fi.exists() || !fi.isReadable())
      {
        if (errorIfNotExist)
        {
          warn_uncond("source '%s' is not a readable file or directory... skipping.\n",s);
        }
      }
      else if (!Config_getBool(EXCLUDE_SYMLINKS) || !fi.isSymLink())
      {
        if (fi.isFile())
        {
          QCString dirPath = fi.dirPath(TRUE).utf8();
          QCString filePath = fi.absFilePath().utf8();
          if (paths && !dirPath.isEmpty())
          {
            paths->insert(dirPath.data());
          }
          //printf("killSet.find(%s)=%d\n",fi.absFilePath().data(),killSet.find(fi.absFilePath())!=killSet.end());
          if (killSet==0 || killSet->find(filePath.data())==killSet->end())
          {
            totalSize+=fi.size()+fi.absFilePath().length()+4; //readFile(&fi,fiList,input);
            //fiList->inSort(new FileInfo(fi));
            QCString name=fi.fileName().utf8();
            //printf("New file %s\n",name.data());
            if (fnMap)
            {
              std::unique_ptr<FileDef> fd { createFileDef(dirPath+"/",name) };
              if (!name.isEmpty())
              {
                FileName *fn = fnMap->add(name,filePath);
                fn->push_back(std::move(fd));
              }
            }
            if (resultList || resultSet)
            {
              if (resultList) resultList->push_back(filePath.data());
              if (resultSet) resultSet->insert(filePath.data());
            }

            if (killSet) killSet->insert(fi.absFilePath().utf8().data());
          }
        }
        else if (fi.isDir()) // readable dir
        {
          totalSize+=readDir(&fi,fnMap,exclSet,patList,
              exclPatList,resultList,resultSet,errorIfNotExist,
              recursive,killSet,paths);
        }
      }
    }
  }
  return totalSize;
}

//----------------------------------------------------------------------------

static void expandAliases()
{
  QDictIterator<QCString> adi(Doxygen::aliasDict);
  QCString *s;
  for (adi.toFirst();(s=adi.current());++adi)
  {
    *s = expandAlias(adi.currentKey(),*s);
  }
}

//----------------------------------------------------------------------------

static void escapeAliases()
{
  QDictIterator<QCString> adi(Doxygen::aliasDict);
  QCString *s;
  for (adi.toFirst();(s=adi.current());++adi)
  {
    QCString value=*s,newValue;
    int in,p=0;
    // for each \n in the alias command value
    while ((in=value.find("\\n",p))!=-1)
    {
      newValue+=value.mid(p,in-p);
      // expand \n's except if \n is part of a built-in command.
      if (value.mid(in,5)!="\\note" &&
          value.mid(in,5)!="\\name" &&
          value.mid(in,10)!="\\namespace" &&
          value.mid(in,14)!="\\nosubgrouping"
         )
      {
        newValue+="\\ilinebr ";
      }
      else
      {
        newValue+="\\n";
      }
      p=in+2;
    }
    newValue+=value.mid(p,value.length()-p);
    *s=newValue;
    p = 0;
    newValue = "";
    while ((in=value.find("^^",p))!=-1)
    {
      newValue+=value.mid(p,in-p);
      newValue+="\\ilinebr ";
      p=in+2;
    }
    newValue+=value.mid(p,value.length()-p);
    *s=newValue;
    //printf("Alias %s has value %s\n",adi.currentKey().data(),s->data());
  }
}

//----------------------------------------------------------------------------

void readAliases()
{
  // add aliases to a dictionary
  Doxygen::aliasDict.setAutoDelete(TRUE);
  const StringVector &aliasList = Config_getList(ALIASES);
  for (const auto &s : aliasList)
  {
    QCString alias=s.c_str();
    if (Doxygen::aliasDict[alias]==0)
    {
      int i=alias.find('=');
      if (i>0)
      {
        QCString name=alias.left(i).stripWhiteSpace();
        QCString value=alias.right(alias.length()-i-1);
        //printf("Alias: found name='%s' value='%s'\n",name.data(),value.data());
        if (!name.isEmpty())
        {
          QCString *dn=Doxygen::aliasDict[name];
          if (dn==0) // insert new alias
          {
            Doxygen::aliasDict.insert(name,new QCString(value));
          }
          else // overwrite previous alias
          {
            *dn=value;
          }
        }
      }
    }
  }
  expandAliases();
  escapeAliases();
}

//----------------------------------------------------------------------------

static void dumpSymbol(FTextStream &t,Definition *d)
{
  QCString anchor;
  if (d->definitionType()==Definition::TypeMember)
  {
    MemberDef *md = dynamic_cast<MemberDef *>(d);
    anchor=":"+md->anchor();
  }
  QCString scope;
  if (d->getOuterScope() && d->getOuterScope()!=Doxygen::globalScope)
  {
    scope = d->getOuterScope()->getOutputFileBase()+Doxygen::htmlFileExtension;
  }
  t << "REPLACE INTO symbols (symbol_id,scope_id,name,file,line) VALUES('"
    << d->getOutputFileBase()+Doxygen::htmlFileExtension+anchor << "','"
    << scope << "','"
    << d->name() << "','"
    << d->getDefFileName() << "','"
    << d->getDefLine()
    << "');" << endl;
}

static void dumpSymbolMap()
{
  QFile f("symbols.sql");
  if (f.open(IO_WriteOnly))
  {
    FTextStream t(&f);
    QDictIterator<DefinitionIntf> di(*Doxygen::symbolMap);
    DefinitionIntf *intf;
    for (;(intf=di.current());++di)
    {
      if (intf->definitionType()==DefinitionIntf::TypeSymbolList) // list of symbols
      {
        DefinitionListIterator dli(*(DefinitionList*)intf);
        Definition *d;
        // for each symbol
        for (dli.toFirst();(d=dli.current());++dli)
        {
          dumpSymbol(t,d);
        }
      }
      else // single symbol
      {
        Definition *d = (Definition *)intf;
        if (d!=Doxygen::globalScope) dumpSymbol(t,d);
      }
    }
  }
}

// print developer options of doxygen
static void devUsage()
{
  msg("Developer parameters:\n");
  msg("  -m          dump symbol map\n");
  msg("  -b          output to wizard\n");
  msg("  -T          activates output generation via Django like template\n");
  msg("  -d <level>  enable a debug level, such as (multiple invocations of -d are possible):\n");
  Debug::printFlags();
}


//----------------------------------------------------------------------------
// print the usage of doxygen

static void usage(const char *name,const char *versionString)
{
  msg("Doxygen version %s\nCopyright Dimitri van Heesch 1997-2019\n\n",versionString);
  msg("You can use doxygen in a number of ways:\n\n");
  msg("1) Use doxygen to generate a template configuration file:\n");
  msg("    %s [-s] -g [configName]\n\n",name);
  msg("2) Use doxygen to update an old configuration file:\n");
  msg("    %s [-s] -u [configName]\n\n",name);
  msg("3) Use doxygen to generate documentation using an existing ");
  msg("configuration file:\n");
  msg("    %s [configName]\n\n",name);
  msg("4) Use doxygen to generate a template file controlling the layout of the\n");
  msg("   generated documentation:\n");
  msg("    %s -l [layoutFileName]\n\n",name);
  msg("    In case layoutFileName is omitted layoutFileName.xml will be used as filename.\n");
  msg("    If - is used for layoutFileName doxygen will write to standard output.\n\n");
  msg("5) Use doxygen to generate a template style sheet file for RTF, HTML or Latex.\n");
  msg("    RTF:        %s -w rtf styleSheetFile\n",name);
  msg("    HTML:       %s -w html headerFile footerFile styleSheetFile [configFile]\n",name);
  msg("    LaTeX:      %s -w latex headerFile footerFile styleSheetFile [configFile]\n\n",name);
  msg("6) Use doxygen to generate a rtf extensions file\n");
  msg("    RTF:   %s -e rtf extensionsFile\n\n",name);
  msg("    If - is used for extensionsFile doxygen will write to standard output.\n\n");
  msg("7) Use doxygen to compare the used configuration file with the template configuration file\n");
  msg("    %s -x [configFile]\n\n",name);
  msg("8) Use doxygen to show a list of built-in emojis.\n");
  msg("    %s -f emoji outputFileName\n\n",name);
  msg("    If - is used for outputFileName doxygen will write to standard output.\n\n");
  msg("If -s is specified the comments of the configuration items in the config file will be omitted.\n");
  msg("If configName is omitted 'Doxyfile' will be used as a default.\n");
  msg("If - is used for configFile doxygen will write / read the configuration to /from standard output / input.\n\n");
  msg("-v print version string\n");
}

//----------------------------------------------------------------------------
// read the argument of option 'c' from the comment argument list and
// update the option index 'optind'.

static const char *getArg(int argc,char **argv,int &optind)
{
  char *s=0;
  if (qstrlen(&argv[optind][2])>0)
    s=&argv[optind][2];
  else if (optind+1<argc && argv[optind+1][0]!='-')
    s=argv[++optind];
  return s;
}

//----------------------------------------------------------------------------

/** @brief /dev/null outline parser */
class NullOutlineParser : public OutlineParserInterface
{
  public:
    void parseInput(const char *, const char *,const std::shared_ptr<Entry> &, ClangTUParser*) {}
    bool needsPreprocessing(const QCString &) const { return FALSE; }
    void parsePrototype(const char *) {}
};


template<class T> std::function< std::unique_ptr<T>() > make_parser_factory()
{
  return []() { return std::make_unique<T>(); };
}

void initDoxygen()
{
  initResources();
  const char *lang = Portable::getenv("LC_ALL");
  if (lang) Portable::setenv("LANG",lang);
  setlocale(LC_ALL,"");
  setlocale(LC_CTYPE,"C"); // to get isspace(0xA0)==0, needed for UTF-8
  setlocale(LC_NUMERIC,"C");

  Portable::correct_path();

  Debug::startTimer();
  Doxygen::parserManager = new ParserManager(            make_parser_factory<NullOutlineParser>(),
                                                         make_parser_factory<FileCodeParser>());
  Doxygen::parserManager->registerParser("c",            make_parser_factory<COutlineParser>(),
                                                         make_parser_factory<CCodeParser>());
  Doxygen::parserManager->registerParser("python",       make_parser_factory<PythonOutlineParser>(),
                                                         make_parser_factory<PythonCodeParser>());
  Doxygen::parserManager->registerParser("fortran",      make_parser_factory<FortranOutlineParser>(),
                                                         make_parser_factory<FortranCodeParser>());
  Doxygen::parserManager->registerParser("fortranfree",  make_parser_factory<FortranOutlineParserFree>(),
                                                         make_parser_factory<FortranCodeParserFree>());
  Doxygen::parserManager->registerParser("fortranfixed", make_parser_factory<FortranOutlineParserFixed>(),
                                                         make_parser_factory<FortranCodeParserFixed>());
  Doxygen::parserManager->registerParser("vhdl",         make_parser_factory<VHDLOutlineParser>(),
                                                         make_parser_factory<VHDLCodeParser>());
  Doxygen::parserManager->registerParser("xml",          make_parser_factory<NullOutlineParser>(),
                                                         make_parser_factory<XMLCodeParser>());
  Doxygen::parserManager->registerParser("sql",          make_parser_factory<NullOutlineParser>(),
                                                         make_parser_factory<SQLCodeParser>());
  Doxygen::parserManager->registerParser("md",           make_parser_factory<MarkdownOutlineParser>(),
                                                         make_parser_factory<FileCodeParser>());

  // register any additional parsers here...

  initDefaultExtensionMapping();
  initClassMemberIndices();
  initNamespaceMemberIndices();
  initFileMemberIndices();

  Doxygen::symbolMap     = new QDict<DefinitionIntf>(50177);
#ifdef USE_LIBCLANG
  Doxygen::clangUsrMap   = new QDict<Definition>(50177);
#endif
  Doxygen::memberNameLinkedMap = new MemberNameLinkedMap;
  Doxygen::functionNameLinkedMap = new MemberNameLinkedMap;
  Doxygen::groupSDict = new GroupSDict(17);
  Doxygen::groupSDict->setAutoDelete(TRUE);
  Doxygen::namespaceSDict = new NamespaceSDict(20);
  Doxygen::namespaceSDict->setAutoDelete(TRUE);
  Doxygen::classSDict = new ClassSDict(1009);
  Doxygen::classSDict->setAutoDelete(TRUE);
  Doxygen::hiddenClasses = new ClassSDict(257);
  Doxygen::hiddenClasses->setAutoDelete(TRUE);
  Doxygen::directories = new DirSDict(17);
  Doxygen::directories->setAutoDelete(TRUE);
  Doxygen::pageSDict = new PageSDict(1009);          // all doc pages
  Doxygen::pageSDict->setAutoDelete(TRUE);
  Doxygen::exampleSDict = new PageSDict(1009);       // all examples
  Doxygen::exampleSDict->setAutoDelete(TRUE);
  Doxygen::memGrpInfoDict.setAutoDelete(TRUE);
  Doxygen::tagDestinationDict.setAutoDelete(TRUE);
  Doxygen::dirRelations.setAutoDelete(TRUE);
  Doxygen::genericsDict = new GenericsSDict;
  Doxygen::indexList = new IndexList;

  // initialisation of these globals depends on
  // configuration switches so we need to postpone these
  Doxygen::globalScope     = 0;
  Doxygen::inputNameLinkedMap   = 0;
  Doxygen::includeNameLinkedMap = 0;
  Doxygen::exampleNameLinkedMap = 0;
  Doxygen::imageNameLinkedMap   = 0;
  Doxygen::dotFileNameLinkedMap = 0;
  Doxygen::mscFileNameLinkedMap = 0;
  Doxygen::diaFileNameLinkedMap = 0;

  /**************************************************************************
   *            Initialize some global constants
   **************************************************************************/

  g_compoundKeywordDict.insert("template class",(void *)8);
  g_compoundKeywordDict.insert("template struct",(void *)8);
  g_compoundKeywordDict.insert("class",(void *)8);
  g_compoundKeywordDict.insert("struct",(void *)8);
  g_compoundKeywordDict.insert("union",(void *)8);
  g_compoundKeywordDict.insert("interface",(void *)8);
  g_compoundKeywordDict.insert("exception",(void *)8);
}

void cleanUpDoxygen()
{
  FormulaManager::instance().clear();
  SectionManager::instance().clear();

  delete Doxygen::indexList;
  delete Doxygen::genericsDict;
  delete Doxygen::inputNameLinkedMap;
  delete Doxygen::includeNameLinkedMap;
  delete Doxygen::exampleNameLinkedMap;
  delete Doxygen::imageNameLinkedMap;
  delete Doxygen::dotFileNameLinkedMap;
  delete Doxygen::mscFileNameLinkedMap;
  delete Doxygen::diaFileNameLinkedMap;
  delete Doxygen::mainPage;
  delete Doxygen::pageSDict;
  delete Doxygen::exampleSDict;
  delete Doxygen::globalScope;
  delete Doxygen::parserManager;
  delete theTranslator;
  delete g_outputList;
  Mappers::freeMappers();

  if (Doxygen::symbolMap)
  {
    // iterate through Doxygen::symbolMap and delete all
    // DefinitionList objects, since they have no owner
    QDictIterator<DefinitionIntf> dli(*Doxygen::symbolMap);
    DefinitionIntf *di;
    for (dli.toFirst();(di=dli.current());)
    {
      if (di->definitionType()==DefinitionIntf::TypeSymbolList)
      {
        DefinitionIntf *tmp = Doxygen::symbolMap->take(dli.currentKey());
        delete (DefinitionList *)tmp;
      }
      else
      {
        ++dli;
      }
    }
  }

  delete Doxygen::memberNameLinkedMap;
  delete Doxygen::functionNameLinkedMap;
  delete Doxygen::groupSDict;
  delete Doxygen::classSDict;
  delete Doxygen::hiddenClasses;
  delete Doxygen::namespaceSDict;
  delete Doxygen::directories;

  DotManager::deleteInstance();

  //delete Doxygen::symbolMap; <- we cannot do this unless all static lists
  //                              (such as Doxygen::namespaceSDict)
  //                              with objects based on Definition are made
  //                              dynamic first
}

static int computeIdealCacheParam(uint v)
{
  //printf("computeIdealCacheParam(v=%u)\n",v);

  int r=0;
  while (v!=0) v>>=1,r++;
  // r = log2(v)

  // convert to a valid cache size value
  return QMAX(0,QMIN(r-16,9));
}

void readConfiguration(int argc, char **argv)
{
  QCString versionString = getFullVersion();

  /**************************************************************************
   *             Handle arguments                                           *
   **************************************************************************/

  int optind=1;
  const char *configName=0;
  const char *layoutName=0;
  const char *debugLabel;
  const char *formatName;
  const char *listName;
  bool genConfig=FALSE;
  bool shortList=FALSE;
  bool diffList=FALSE;
  bool updateConfig=FALSE;
  int retVal;
  while (optind<argc && argv[optind][0]=='-' &&
               (isalpha(argv[optind][1]) || argv[optind][1]=='?' ||
                argv[optind][1]=='-')
        )
  {
    switch(argv[optind][1])
    {
      case 'g':
        genConfig=TRUE;
        break;
      case 'l':
        if (optind+1>=argc)
        {
          layoutName="DoxygenLayout.xml";
        }
        else
        {
          layoutName=argv[optind+1];
        }
        writeDefaultLayoutFile(layoutName);
        cleanUpDoxygen();
        exit(0);
        break;
      case 'd':
        debugLabel=getArg(argc,argv,optind);
        if (!debugLabel)
        {
          devUsage();
          cleanUpDoxygen();
          exit(0);
        }
        retVal = Debug::setFlag(debugLabel);
        if (!retVal)
        {
          err("option \"-d\" has unknown debug specifier: \"%s\".\n",debugLabel);
          devUsage();
          cleanUpDoxygen();
          exit(1);
        }
        break;
      case 'x':
        diffList=TRUE;
        break;
      case 's':
        shortList=TRUE;
        break;
      case 'u':
        updateConfig=TRUE;
        break;
      case 'e':
        formatName=getArg(argc,argv,optind);
        if (!formatName)
        {
          err("option \"-e\" is missing format specifier rtf.\n");
          cleanUpDoxygen();
          exit(1);
        }
        if (qstricmp(formatName,"rtf")==0)
        {
          if (optind+1>=argc)
          {
            err("option \"-e rtf\" is missing an extensions file name\n");
            cleanUpDoxygen();
            exit(1);
          }
          QFile f;
          if (openOutputFile(argv[optind+1],f))
          {
            RTFGenerator::writeExtensionsFile(f);
          }
          cleanUpDoxygen();
          exit(0);
        }
        err("option \"-e\" has invalid format specifier.\n");
        cleanUpDoxygen();
        exit(1);
        break;
      case 'f':
        listName=getArg(argc,argv,optind);
        if (!listName)
        {
          err("option \"-f\" is missing list specifier.\n");
          cleanUpDoxygen();
          exit(1);
        }
        if (qstricmp(listName,"emoji")==0)
        {
          if (optind+1>=argc)
          {
            err("option \"-f emoji\" is missing an output file name\n");
            cleanUpDoxygen();
            exit(1);
          }
          QFile f;
          if (openOutputFile(argv[optind+1],f))
          {
            EmojiEntityMapper::instance()->writeEmojiFile(f);
          }
          cleanUpDoxygen();
          exit(0);
        }
        err("option \"-f\" has invalid list specifier.\n");
        cleanUpDoxygen();
        exit(1);
        break;
      case 'w':
        formatName=getArg(argc,argv,optind);
        if (!formatName)
        {
          err("option \"-w\" is missing format specifier rtf, html or latex\n");
          cleanUpDoxygen();
          exit(1);
        }
        if (qstricmp(formatName,"rtf")==0)
        {
          if (optind+1>=argc)
          {
            err("option \"-w rtf\" is missing a style sheet file name\n");
            cleanUpDoxygen();
            exit(1);
          }
          QFile f;
          if (openOutputFile(argv[optind+1],f))
          {
            RTFGenerator::writeStyleSheetFile(f);
          }
          cleanUpDoxygen();
          exit(1);
        }
        else if (qstricmp(formatName,"html")==0)
        {
          Config::init();
          if (optind+4<argc || QFileInfo("Doxyfile").exists())
             // explicit config file mentioned or default found on disk
          {
            QCString df = optind+4<argc ? argv[optind+4] : QCString("Doxyfile");
            if (!Config::parse(df)) // parse the config file
            {
              err("error opening or reading configuration file %s!\n",argv[optind+4]);
              cleanUpDoxygen();
              exit(1);
            }
          }
          if (optind+3>=argc)
          {
            err("option \"-w html\" does not have enough arguments\n");
            cleanUpDoxygen();
            exit(1);
          }
          Config::postProcess(TRUE);
          Config::checkAndCorrect();

          QCString outputLanguage=Config_getEnum(OUTPUT_LANGUAGE);
          if (!setTranslator(outputLanguage))
          {
            warn_uncond("Output language %s not supported! Using English instead.\n", outputLanguage.data());
          }

          QFile f;
          if (openOutputFile(argv[optind+1],f))
          {
            HtmlGenerator::writeHeaderFile(f, argv[optind+3]);
          }
          f.close();
          if (openOutputFile(argv[optind+2],f))
          {
            HtmlGenerator::writeFooterFile(f);
          }
          f.close();
          if (openOutputFile(argv[optind+3],f))
          {
            HtmlGenerator::writeStyleSheetFile(f);
          }
          cleanUpDoxygen();
          exit(0);
        }
        else if (qstricmp(formatName,"latex")==0)
        {
          Config::init();
          if (optind+4<argc || QFileInfo("Doxyfile").exists())
          {
            QCString df = optind+4<argc ? argv[optind+4] : QCString("Doxyfile");
            if (!Config::parse(df))
            {
              err("error opening or reading configuration file %s!\n",argv[optind+4]);
              cleanUpDoxygen();
              exit(1);
            }
          }
          if (optind+3>=argc)
          {
            err("option \"-w latex\" does not have enough arguments\n");
            cleanUpDoxygen();
            exit(1);
          }
          Config::postProcess(TRUE);
          Config::checkAndCorrect();

          QCString outputLanguage=Config_getEnum(OUTPUT_LANGUAGE);
          if (!setTranslator(outputLanguage))
          {
            warn_uncond("Output language %s not supported! Using English instead.\n", outputLanguage.data());
          }

          QFile f;
          if (openOutputFile(argv[optind+1],f))
          {
            LatexGenerator::writeHeaderFile(f);
          }
          f.close();
          if (openOutputFile(argv[optind+2],f))
          {
            LatexGenerator::writeFooterFile(f);
          }
          f.close();
          if (openOutputFile(argv[optind+3],f))
          {
            LatexGenerator::writeStyleSheetFile(f);
          }
          cleanUpDoxygen();
          exit(0);
        }
        else
        {
          err("Illegal format specifier \"%s\": should be one of rtf, html or latex\n",formatName);
          cleanUpDoxygen();
          exit(1);
        }
        break;
      case 'm':
        g_dumpSymbolMap = TRUE;
        break;
      case 'v':
        msg("%s\n",versionString.data());
        cleanUpDoxygen();
        exit(0);
        break;
      case '-':
        if (qstrcmp(&argv[optind][2],"help")==0)
        {
          usage(argv[0],versionString);
          exit(0);
        }
        else if (qstrcmp(&argv[optind][2],"version")==0)
        {
          msg("%s\n",versionString.data());
          cleanUpDoxygen();
          exit(0);
        }
        else
        {
          err("Unknown option \"-%s\"\n",&argv[optind][1]);
          usage(argv[0],versionString);
          exit(1);
        }
        break;
      case 'b':
        setvbuf(stdout,NULL,_IONBF,0);
        Doxygen::outputToWizard=TRUE;
        break;
      case 'T':
        msg("Warning: this option activates output generation via Django like template files. "
            "This option is scheduled for doxygen 2.0, is currently incomplete and highly experimental! "
            "Only use if you are a doxygen developer\n");
        g_useOutputTemplate=TRUE;
        break;
      case 'h':
      case '?':
        usage(argv[0],versionString);
        exit(0);
        break;
      default:
        err("Unknown option \"-%c\"\n",argv[optind][1]);
        usage(argv[0],versionString);
        exit(1);
    }
    optind++;
  }

  /**************************************************************************
   *            Parse or generate the config file                           *
   **************************************************************************/

  Config::init();

  QFileInfo configFileInfo1("Doxyfile"),configFileInfo2("doxyfile");
  if (optind>=argc)
  {
    if (configFileInfo1.exists())
    {
      configName="Doxyfile";
    }
    else if (configFileInfo2.exists())
    {
      configName="doxyfile";
    }
    else if (genConfig)
    {
      configName="Doxyfile";
    }
    else
    {
      err("Doxyfile not found and no input file specified!\n");
      usage(argv[0],versionString);
      exit(1);
    }
  }
  else
  {
    QFileInfo fi(argv[optind]);
    if (fi.exists() || qstrcmp(argv[optind],"-")==0 || genConfig)
    {
      configName=argv[optind];
    }
    else
    {
      err("configuration file %s not found!\n",argv[optind]);
      usage(argv[0],versionString);
      exit(1);
    }
  }

  if (genConfig && g_useOutputTemplate)
  {
    generateTemplateFiles("templates");
    cleanUpDoxygen();
    exit(0);
  }

  if (genConfig)
  {
    generateConfigFile(configName,shortList);
    cleanUpDoxygen();
    exit(0);
  }

  if (!Config::parse(configName,updateConfig))
  {
    err("could not open or read configuration file %s!\n",configName);
    cleanUpDoxygen();
    exit(1);
  }

  if (diffList)
  {
    compareDoxyfile();
    cleanUpDoxygen();
    exit(0);
  }

  if (updateConfig)
  {
    generateConfigFile(configName,shortList,TRUE);
    cleanUpDoxygen();
    exit(0);
  }

  /* Perlmod wants to know the path to the config file.*/
  QFileInfo configFileInfo(configName);
  setPerlModDoxyfile(configFileInfo.absFilePath().data());

}

/** check and resolve config options */
void checkConfiguration()
{

  Config::postProcess(FALSE);
  Config::checkAndCorrect();
  initWarningFormat();
}

/** adjust globals that depend on configuration settings. */
void adjustConfiguration()
{
  Doxygen::globalScope = createNamespaceDef("<globalScope>",1,1,"<globalScope>");
  Doxygen::inputNameLinkedMap = new FileNameLinkedMap;
  Doxygen::includeNameLinkedMap = new FileNameLinkedMap;
  Doxygen::exampleNameLinkedMap = new FileNameLinkedMap;
  Doxygen::imageNameLinkedMap = new FileNameLinkedMap;
  Doxygen::dotFileNameLinkedMap = new FileNameLinkedMap;
  Doxygen::mscFileNameLinkedMap = new FileNameLinkedMap;
  Doxygen::diaFileNameLinkedMap = new FileNameLinkedMap;

  QCString outputLanguage=Config_getEnum(OUTPUT_LANGUAGE);
  if (!setTranslator(outputLanguage))
  {
    warn_uncond("Output language %s not supported! Using English instead.\n",
       outputLanguage.data());
  }

  /* Set the global html file extension. */
  Doxygen::htmlFileExtension = Config_getString(HTML_FILE_EXTENSION);


  Doxygen::parseSourcesNeeded = Config_getBool(CALL_GRAPH) ||
                                Config_getBool(CALLER_GRAPH) ||
                                Config_getBool(REFERENCES_RELATION) ||
                                Config_getBool(REFERENCED_BY_RELATION);

  /**************************************************************************
   *            Add custom extension mappings
   **************************************************************************/

  const StringVector &extMaps = Config_getList(EXTENSION_MAPPING);
  for (const auto &mapping : extMaps)
  {
    QCString mapStr = mapping.c_str();
    int i=mapStr.find('=');
    if (i==-1)
    {
      continue;
    }
    else
    {
      QCString ext = mapStr.left(i).stripWhiteSpace().lower();
      QCString language = mapStr.mid(i+1).stripWhiteSpace().lower();
      if (ext.isEmpty() || language.isEmpty())
      {
        continue;
      }

      if (!updateLanguageMapping(ext,language))
      {
        err("Failed to map file extension '%s' to unsupported language '%s'.\n"
            "Check the EXTENSION_MAPPING setting in the config file.\n",
            ext.data(),language.data());
      }
      else
      {
        msg("Adding custom extension mapping: .%s will be treated as language %s\n",
            ext.data(),language.data());
      }
    }
  }

  // add predefined macro name to a dictionary
  const StringVector &expandAsDefinedList =Config_getList(EXPAND_AS_DEFINED);
  for (const auto &s : expandAsDefinedList)
  {
    Doxygen::expandAsDefinedSet.insert(s.c_str());
  }

  // read aliases and store them in a dictionary
  readAliases();

  // store number of spaces in a tab into Doxygen::spaces
  int tabSize = Config_getInt(TAB_SIZE);
  Doxygen::spaces.resize(tabSize+1);
  int sp;for (sp=0;sp<tabSize;sp++) Doxygen::spaces.at(sp)=' ';
  Doxygen::spaces.at(tabSize)='\0';
}

#ifdef HAS_SIGNALS
static void stopDoxygen(int)
{
  QDir thisDir;
  msg("Cleaning up...\n");
  if (!Doxygen::entryDBFileName.isEmpty())
  {
    thisDir.remove(Doxygen::entryDBFileName);
  }
  if (!Doxygen::objDBFileName.isEmpty())
  {
    thisDir.remove(Doxygen::objDBFileName);
  }
  if (!Doxygen::filterDBFileName.isEmpty())
  {
    thisDir.remove(Doxygen::filterDBFileName);
  }
  killpg(0,SIGINT);
  exit(1);
}
#endif

static void writeTagFile()
{
  QCString generateTagFile = Config_getString(GENERATE_TAGFILE);
  if (generateTagFile.isEmpty()) return;

  QFile tag(generateTagFile);
  if (!tag.open(IO_WriteOnly))
  {
    err("cannot open tag file %s for writing\n",
        generateTagFile.data()
       );
    return;
  }
  FTextStream tagFile(&tag);
  tagFile << "<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>" << endl;
  tagFile << "<tagfile doxygen_version=\"" << getDoxygenVersion() << "\"";
  if (strlen(getGitVersion())>0)
  {
    tagFile << " doxygen_gitid=\"" << getGitVersion() << "\"";
  }
  tagFile << ">" << endl;

  // for each file
  for (const auto &fn : *Doxygen::inputNameLinkedMap)
  {
    for (const auto &fd : *fn)
    {
      if (fd->isLinkableInProject()) fd->writeTagFile(tagFile);
    }
  }
  // for each class
  ClassSDict::Iterator cli(*Doxygen::classSDict);
  ClassDef *cd;
  for ( ; (cd=cli.current()) ; ++cli )
  {
    if (cd->isLinkableInProject()) cd->writeTagFile(tagFile);
  }
  // for each namespace
  NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
  NamespaceDef *nd;
  for ( ; (nd=nli.current()) ; ++nli )
  {
    if (nd->isLinkableInProject()) nd->writeTagFile(tagFile);
  }
  // for each group
  GroupSDict::Iterator gli(*Doxygen::groupSDict);
  GroupDef *gd;
  for (gli.toFirst();(gd=gli.current());++gli)
  {
    if (gd->isLinkableInProject()) gd->writeTagFile(tagFile);
  }
  // for each page
  PageSDict::Iterator pdi(*Doxygen::pageSDict);
  PageDef *pd=0;
  for (pdi.toFirst();(pd=pdi.current());++pdi)
  {
    if (pd->isLinkableInProject()) pd->writeTagFile(tagFile);
  }
  if (Doxygen::mainPage) Doxygen::mainPage->writeTagFile(tagFile);

  /*
  if (Doxygen::mainPage && !Config_getString(GENERATE_TAGFILE).isEmpty())
  {
    tagFile << "  <compound kind=\"page\">" << endl
                     << "    <name>"
                     << convertToXML(Doxygen::mainPage->name())
                     << "</name>" << endl
                     << "    <title>"
                     << convertToXML(Doxygen::mainPage->title())
                     << "</title>" << endl
                     << "    <filename>"
                     << convertToXML(Doxygen::mainPage->getOutputFileBase())
                     << Doxygen::htmlFileExtension
                     << "</filename>" << endl;

    mainPage->writeDocAnchorsToTagFile();
    tagFile << "  </compound>" << endl;
  }
  */

  tagFile << "</tagfile>" << endl;
}

static void exitDoxygen()
{
  if (!g_successfulRun)  // premature exit
  {
    QDir thisDir;
    msg("Exiting...\n");
    if (!Doxygen::entryDBFileName.isEmpty())
    {
      thisDir.remove(Doxygen::entryDBFileName);
    }
    if (!Doxygen::objDBFileName.isEmpty())
    {
      thisDir.remove(Doxygen::objDBFileName);
    }
    if (!Doxygen::filterDBFileName.isEmpty())
    {
      thisDir.remove(Doxygen::filterDBFileName);
    }
  }
}

static QCString createOutputDirectory(const QCString &baseDirName,
                                  const QCString &formatDirName,
                                  const char *defaultDirName)
{
  QCString result = formatDirName;
  if (result.isEmpty())
  {
    result = baseDirName + defaultDirName;
  }
  else if (formatDirName[0]!='/' && (formatDirName.length()==1 || formatDirName[1]!=':'))
  {
    result.prepend(baseDirName+'/');
  }
  QDir formatDir(result);
  if (!formatDir.exists() && !formatDir.mkdir(result))
  {
    err("Could not create output directory %s\n", result.data());
    cleanUpDoxygen();
    exit(1);
  }
  return result;
}

static QCString getQchFileName()
{
  QCString const & qchFile = Config_getString(QCH_FILE);
  if (!qchFile.isEmpty())
  {
    return qchFile;
  }

  QCString const & projectName = Config_getString(PROJECT_NAME);
  QCString const & versionText = Config_getString(PROJECT_NUMBER);

  return QCString("../qch/")
      + (projectName.isEmpty() ? QCString("index") : projectName)
      + (versionText.isEmpty() ? QCString("") : QCString("-") + versionText)
      + QCString(".qch");
}

void searchInputFiles()
{
  StringUnorderedSet killSet;

  const StringVector &exclPatterns = Config_getList(EXCLUDE_PATTERNS);
  bool alwaysRecursive = Config_getBool(RECURSIVE);
  StringUnorderedSet excludeNameSet;

  // gather names of all files in the include path
  g_s.begin("Searching for include files...\n");
  killSet.clear();
  const StringVector &includePathList = Config_getList(INCLUDE_PATH);
  for (const auto &s : includePathList)
  {
    size_t plSize = Config_getList(INCLUDE_FILE_PATTERNS).size();
    const StringVector &pl = plSize==0 ? Config_getList(FILE_PATTERNS) :
                                         Config_getList(INCLUDE_FILE_PATTERNS);
    readFileOrDirectory(s.c_str(),                     // s
                        Doxygen::includeNameLinkedMap, // fnDict
                        0,                             // exclSet
                        &pl,                           // patList
                        &exclPatterns,                 // exclPatList
                        0,                             // resultList
                        0,                             // resultSet
                        alwaysRecursive,               // recursive
                        TRUE,                          // errorIfNotExist
                        &killSet);                     // killSet
  }
  g_s.end();

  g_s.begin("Searching for example files...\n");
  killSet.clear();
  const StringVector &examplePathList = Config_getList(EXAMPLE_PATH);
  for (const auto &s : examplePathList)
  {
    readFileOrDirectory(s.c_str(),                                              // s
                        Doxygen::exampleNameLinkedMap,                          // fnDict
                        0,                                                      // exclSet
                        &Config_getList(EXAMPLE_PATTERNS),                      // patList
                        0,                                                      // exclPatList
                        0,                                                      // resultList
                        0,                                                      // resultSet
                        (alwaysRecursive || Config_getBool(EXAMPLE_RECURSIVE)), // recursive
                        TRUE,                                                   // errorIfNotExist
                        &killSet);                                              // killSet
  }
  g_s.end();

  g_s.begin("Searching for images...\n");
  killSet.clear();
  const StringVector &imagePathList=Config_getList(IMAGE_PATH);
  for (const auto &s : imagePathList)
  {
    readFileOrDirectory(s.c_str(),                        // s
                        Doxygen::imageNameLinkedMap,      // fnDict
                        0,                                // exclSet
                        0,                                // patList
                        0,                                // exclPatList
                        0,                                // resultList
                        0,                                // resultSet
                        alwaysRecursive,                  // recursive
                        TRUE,                             // errorIfNotExist
                        &killSet);                        // killSet
  }
  g_s.end();

  g_s.begin("Searching for dot files...\n");
  killSet.clear();
  const StringVector &dotFileList=Config_getList(DOTFILE_DIRS);
  for (const auto &s : dotFileList)
  {
    readFileOrDirectory(s.c_str(),                      // s
                        Doxygen::dotFileNameLinkedMap,  // fnDict
                        0,                              // exclSet
                        0,                              // patList
                        0,                              // exclPatList
                        0,                              // resultList
                        0,                              // resultSet
                        alwaysRecursive,                // recursive
                        TRUE,                           // errorIfNotExist
                        &killSet);                      // killSet
  }
  g_s.end();

  g_s.begin("Searching for msc files...\n");
  killSet.clear();
  const StringVector &mscFileList=Config_getList(MSCFILE_DIRS);
  for (const auto &s : mscFileList)
  {
    readFileOrDirectory(s.c_str(),                       // s
                        Doxygen::mscFileNameLinkedMap,   // fnDict
                        0,                               // exclSet
                        0,                               // patList
                        0,                               // exclPatList
                        0,                               // resultList
                        0,                               // resultSet
                        alwaysRecursive,                 // recursive
                        TRUE,                            // errorIfNotExist
                        &killSet);                       // killSet
  }
  g_s.end();

  g_s.begin("Searching for dia files...\n");
  killSet.clear();
  const StringVector &diaFileList=Config_getList(DIAFILE_DIRS);
  for (const auto &s : diaFileList)
  {
    readFileOrDirectory(s.c_str(),                         // s
                        Doxygen::diaFileNameLinkedMap,     // fnDict
                        0,                                 // exclSet
                        0,                                 // patList
                        0,                                 // exclPatList
                        0,                                 // resultList
                        0,                                 // resultSet
                        alwaysRecursive,                   // recursive
                        TRUE,                              // errorIfNotExist
                        &killSet);                         // killSet
  }
  g_s.end();

  g_s.begin("Searching for files to exclude\n");
  const StringVector &excludeList = Config_getList(EXCLUDE);
  for (const auto &s : excludeList)
  {
    readFileOrDirectory(s.c_str(),                          // s
                        0,                                  // fnDict
                        0,                                  // exclSet
                        &Config_getList(FILE_PATTERNS),     // patList
                        0,                                  // exclPatList
                        0,                                  // resultList
                        &excludeNameSet,                    // resultSet
                        alwaysRecursive,                    // recursive
                        FALSE);                             // errorIfNotExist
  }
  g_s.end();

  /**************************************************************************
   *             Determine Input Files                                      *
   **************************************************************************/

  g_s.begin("Searching INPUT for files to process...\n");
  killSet.clear();
  Doxygen::inputPaths.clear();
  const StringVector &inputList=Config_getList(INPUT);
  for (const auto &s : inputList)
  {
    QCString path=s.c_str();
    uint l = path.length();
    if (l>0)
    {
      // strip trailing slashes
      if (path.at(l-1)=='\\' || path.at(l-1)=='/') path=path.left(l-1);

      readFileOrDirectory(
          path,                               // s
          Doxygen::inputNameLinkedMap,        // fnDict
          &excludeNameSet,                    // exclSet
          &Config_getList(FILE_PATTERNS),     // patList
          &exclPatterns,                      // exclPatList
          &g_inputFiles,                      // resultList
          0,                                  // resultSet
          alwaysRecursive,                    // recursive
          TRUE,                               // errorIfNotExist
          &killSet,                           // killSet
          &Doxygen::inputPaths);              // paths
    }
  }
  std::sort(Doxygen::inputNameLinkedMap->begin(),
            Doxygen::inputNameLinkedMap->end(),
            [](const auto &f1,const auto &f2)
            {
              return Config_getBool(FULL_PATH_NAMES) ?
                     qstricmp(f1->fullName(),f2->fullName())<0 :
                     qstricmp(f1->fileName(),f2->fileName())<0;
            });
  g_s.end();
}


void parseInput()
{
  atexit(exitDoxygen);

#if USE_LIBCLANG
  Doxygen::clangAssistedParsing = Config_getBool(CLANG_ASSISTED_PARSING);
#endif

  // we would like to show the versionString earlier, but we first have to handle the configuration file
  // to know the value of the QUIET setting.
  QCString versionString = getFullVersion();
  msg("Doxygen version used: %s\n",versionString.data());

  /**************************************************************************
   *            Make sure the output directory exists
   **************************************************************************/
  QCString outputDirectory = Config_getString(OUTPUT_DIRECTORY);
  if (outputDirectory.isEmpty())
  {
    outputDirectory = Config_updateString(OUTPUT_DIRECTORY,QDir::currentDirPath().utf8());
  }
  else
  {
    QDir dir(outputDirectory);
    if (!dir.exists())
    {
      dir.setPath(QDir::currentDirPath());
      if (!dir.mkdir(outputDirectory))
      {
        err("tag OUTPUT_DIRECTORY: Output directory '%s' does not "
            "exist and cannot be created\n",outputDirectory.data());
        cleanUpDoxygen();
        exit(1);
      }
      else
      {
        msg("Notice: Output directory '%s' does not exist. "
            "I have created it for you.\n", outputDirectory.data());
      }
      dir.cd(outputDirectory);
    }
    outputDirectory = Config_updateString(OUTPUT_DIRECTORY,dir.absPath().utf8());
  }

  /**************************************************************************
   *            Initialize global lists and dictionaries
   **************************************************************************/

  // also scale lookup cache with SYMBOL_CACHE_SIZE
  int cacheSize = Config_getInt(LOOKUP_CACHE_SIZE);
  if (cacheSize<0) cacheSize=0;
  if (cacheSize>9) cacheSize=9;
  uint lookupSize = 65536 << cacheSize;
  Doxygen::lookupCache = new Cache<std::string,LookupInfo>(lookupSize);

#ifdef HAS_SIGNALS
  signal(SIGINT, stopDoxygen);
#endif

  uint pid = Portable::pid();
  Doxygen::objDBFileName.sprintf("doxygen_objdb_%d.tmp",pid);
  Doxygen::objDBFileName.prepend(outputDirectory+"/");
  Doxygen::entryDBFileName.sprintf("doxygen_entrydb_%d.tmp",pid);
  Doxygen::entryDBFileName.prepend(outputDirectory+"/");
  Doxygen::filterDBFileName.sprintf("doxygen_filterdb_%d.tmp",pid);
  Doxygen::filterDBFileName.prepend(outputDirectory+"/");

  /**************************************************************************
   *            Check/create output directories                             *
   **************************************************************************/

  QCString htmlOutput;
  bool generateHtml = Config_getBool(GENERATE_HTML);
  if (generateHtml || g_useOutputTemplate /* TODO: temp hack */)
  {
    htmlOutput = createOutputDirectory(outputDirectory,Config_getString(HTML_OUTPUT),"/html");
    Config_updateString(HTML_OUTPUT,htmlOutput);
  }

  QCString docbookOutput;
  bool generateDocbook = Config_getBool(GENERATE_DOCBOOK);
  if (generateDocbook)
  {
    docbookOutput = createOutputDirectory(outputDirectory,Config_getString(DOCBOOK_OUTPUT),"/docbook");
    Config_updateString(DOCBOOK_OUTPUT,docbookOutput);
  }

  QCString xmlOutput;
  bool generateXml = Config_getBool(GENERATE_XML);
  if (generateXml)
  {
    xmlOutput = createOutputDirectory(outputDirectory,Config_getString(XML_OUTPUT),"/xml");
    Config_updateString(XML_OUTPUT,xmlOutput);
  }

  QCString latexOutput;
  bool generateLatex = Config_getBool(GENERATE_LATEX);
  if (generateLatex)
  {
    latexOutput = createOutputDirectory(outputDirectory,Config_getString(LATEX_OUTPUT), "/latex");
    Config_updateString(LATEX_OUTPUT,latexOutput);
  }

  QCString rtfOutput;
  bool generateRtf = Config_getBool(GENERATE_RTF);
  if (generateRtf)
  {
    rtfOutput = createOutputDirectory(outputDirectory,Config_getString(RTF_OUTPUT),"/rtf");
    Config_updateString(RTF_OUTPUT,rtfOutput);
  }

  QCString manOutput;
  bool generateMan = Config_getBool(GENERATE_MAN);
  if (generateMan)
  {
    manOutput = createOutputDirectory(outputDirectory,Config_getString(MAN_OUTPUT),"/man");
    Config_updateString(MAN_OUTPUT,manOutput);
  }

#if USE_SQLITE3
  QCString sqlOutput;
  bool generateSql = Config_getBool(GENERATE_SQLITE3);
  if (generateSql)
  {
    sqlOutput = createOutputDirectory(outputDirectory,Config_getString(SQLITE3_OUTPUT),"/sqlite3");
    Config_updateString(SQLITE3_OUTPUT,sqlOutput);
  }
#endif

  if (Config_getBool(HAVE_DOT))
  {
    QCString curFontPath = Config_getString(DOT_FONTPATH);
    if (curFontPath.isEmpty())
    {
      Portable::getenv("DOTFONTPATH");
      QCString newFontPath = ".";
      if (!curFontPath.isEmpty())
      {
        newFontPath+=Portable::pathListSeparator();
        newFontPath+=curFontPath;
      }
      Portable::setenv("DOTFONTPATH",newFontPath);
    }
    else
    {
      Portable::setenv("DOTFONTPATH",curFontPath);
    }
  }



  /**************************************************************************
   *             Handle layout file                                         *
   **************************************************************************/

  LayoutDocManager::instance().init();
  QCString layoutFileName = Config_getString(LAYOUT_FILE);
  bool defaultLayoutUsed = FALSE;
  if (layoutFileName.isEmpty())
  {
    layoutFileName = Config_updateString(LAYOUT_FILE,"DoxygenLayout.xml");
    defaultLayoutUsed = TRUE;
  }

  QFile layoutFile(layoutFileName);
  if (layoutFile.open(IO_ReadOnly))
  {
    msg("Parsing layout file %s...\n",layoutFileName.data());
    LayoutDocManager::instance().parse(layoutFileName);
  }
  else if (!defaultLayoutUsed)
  {
    warn_uncond("failed to open layout file '%s' for reading!\n",layoutFileName.data());
  }

  /**************************************************************************
   *             Read and preprocess input                                  *
   **************************************************************************/

  // prevent search in the output directories
  StringVector exclPatterns = Config_getList(EXCLUDE_PATTERNS);
  if (generateHtml)    exclPatterns.push_back(htmlOutput.data());
  if (generateDocbook) exclPatterns.push_back(docbookOutput.data());
  if (generateXml)     exclPatterns.push_back(xmlOutput.data());
  if (generateLatex)   exclPatterns.push_back(latexOutput.data());
  if (generateRtf)     exclPatterns.push_back(rtfOutput.data());
  if (generateMan)     exclPatterns.push_back(manOutput.data());
  Config_updateList(EXCLUDE_PATTERNS,exclPatterns);

  searchInputFiles();

  // Notice: the order of the function calls below is very important!

  if (Config_getBool(GENERATE_HTML) && !Config_getBool(USE_MATHJAX))
  {
    FormulaManager::instance().readFormulas(Config_getString(HTML_OUTPUT));
  }
  if (Config_getBool(GENERATE_RTF))
  {
    // in case GENERRATE_HTML is set we just have to compare, both repositories should be identical
    FormulaManager::instance().readFormulas(Config_getString(RTF_OUTPUT),
                              Config_getBool(GENERATE_HTML) &&
                              !Config_getBool(USE_MATHJAX));
  }
  if (Config_getBool(GENERATE_DOCBOOK))
  {
    // in case GENERRATE_HTML is set we just have to compare, both repositories should be identical
    FormulaManager::instance().readFormulas(Config_getString(DOCBOOK_OUTPUT),
                         (Config_getBool(GENERATE_HTML) &&
                         !Config_getBool(USE_MATHJAX)) ||
                         Config_getBool(GENERATE_RTF));
  }

  /**************************************************************************
   *             Handle Tag Files                                           *
   **************************************************************************/

  std::shared_ptr<Entry> root = std::make_shared<Entry>();
  msg("Reading and parsing tag files\n");

  const StringVector &tagFileList = Config_getList(TAGFILES);
  for (const auto &s : tagFileList)
  {
    readTagFile(root,s.c_str());
  }

  /**************************************************************************
   *             Parse source files                                         *
   **************************************************************************/

  addSTLSupport(root);

  g_s.begin("Parsing files\n");
  if (Config_getInt(NUM_PROC_THREADS)==1)
  {
    parseFilesSingleThreading(root);
  }
  else
  {
    parseFilesMultiThreading(root);
  }
  g_s.end();

  /**************************************************************************
   *             Gather information                                         *
   **************************************************************************/

  g_s.begin("Building macro definition list...\n");
  buildDefineList();
  g_s.end();

  g_s.begin("Building group list...\n");
  buildGroupList(root.get());
  organizeSubGroups(root.get());
  g_s.end();

  g_s.begin("Building directory list...\n");
  buildDirectories();
  findDirDocumentation(root.get());
  g_s.end();

  g_s.begin("Building namespace list...\n");
  buildNamespaceList(root.get());
  findUsingDirectives(root.get());
  g_s.end();

  g_s.begin("Building file list...\n");
  buildFileList(root.get());
  g_s.end();
  //generateFileTree();

  g_s.begin("Building class list...\n");
  buildClassList(root.get());
  g_s.end();

  // build list of using declarations here (global list)
  buildListOfUsingDecls(root.get());
  g_s.end();

  g_s.begin("Computing nesting relations for classes...\n");
  resolveClassNestingRelations();
  g_s.end();
  // 1.8.2-20121111: no longer add nested classes to the group as well
  //distributeClassGroupRelations();

  // calling buildClassList may result in cached relations that
  // become invalid after resolveClassNestingRelations(), that's why
  // we need to clear the cache here
  Doxygen::lookupCache->clear();
  // we don't need the list of using declaration anymore
  g_usingDeclarations.clear();

  g_s.begin("Associating documentation with classes...\n");
  buildClassDocList(root.get());

  g_s.begin("Building example list...\n");
  buildExampleList(root.get());
  g_s.end();

  g_s.begin("Searching for enumerations...\n");
  findEnums(root.get());
  g_s.end();

  // Since buildVarList calls isVarWithConstructor
  // and this calls getResolvedClass we need to process
  // typedefs first so the relations between classes via typedefs
  // are properly resolved. See bug 536385 for an example.
  g_s.begin("Searching for documented typedefs...\n");
  buildTypedefList(root.get());
  g_s.end();

  if (Config_getBool(OPTIMIZE_OUTPUT_SLICE))
  {
    g_s.begin("Searching for documented sequences...\n");
    buildSequenceList(root.get());
    g_s.end();

    g_s.begin("Searching for documented dictionaries...\n");
    buildDictionaryList(root.get());
    g_s.end();
  }

  g_s.begin("Searching for members imported via using declarations...\n");
  // this should be after buildTypedefList in order to properly import
  // used typedefs
  findUsingDeclarations(root.get());
  g_s.end();

  g_s.begin("Searching for included using directives...\n");
  findIncludedUsingDirectives();
  g_s.end();

  g_s.begin("Searching for documented variables...\n");
  buildVarList(root.get());
  g_s.end();

  g_s.begin("Building interface member list...\n");
  buildInterfaceAndServiceList(root.get()); // UNO IDL

  g_s.begin("Building member list...\n"); // using class info only !
  buildFunctionList(root.get());
  g_s.end();

  g_s.begin("Searching for friends...\n");
  findFriends();
  g_s.end();

  g_s.begin("Searching for documented defines...\n");
  findDefineDocumentation(root.get());
  g_s.end();

  g_s.begin("Computing class inheritance relations...\n");
  findClassEntries(root.get());
  findInheritedTemplateInstances();
  g_s.end();

  g_s.begin("Computing class usage relations...\n");
  findUsedTemplateInstances();
  g_s.end();

  if (Config_getBool(INLINE_SIMPLE_STRUCTS))
  {
    g_s.begin("Searching for tag less structs...\n");
    findTagLessClasses();
    g_s.end();
  }

  g_s.begin("Flushing cached template relations that have become invalid...\n");
  flushCachedTemplateRelations();
  g_s.end();

  g_s.begin("Computing class relations...\n");
  computeTemplateClassRelations();
  flushUnresolvedRelations();
  if (Config_getBool(OPTIMIZE_OUTPUT_VHDL))
  {
    VhdlDocGen::computeVhdlComponentRelations();
  }
  computeClassRelations();
  g_classEntries.clear();
  g_s.end();

  g_s.begin("Add enum values to enums...\n");
  addEnumValuesToEnums(root.get());
  findEnumDocumentation(root.get());
  g_s.end();

  g_s.begin("Searching for member function documentation...\n");
  findObjCMethodDefinitions(root.get());
  findMemberDocumentation(root.get()); // may introduce new members !
  findUsingDeclImports(root.get()); // may introduce new members !

  transferRelatedFunctionDocumentation();
  transferFunctionDocumentation();
  g_s.end();

  // moved to after finding and copying documentation,
  // as this introduces new members see bug 722654
  g_s.begin("Creating members for template instances...\n");
  createTemplateInstanceMembers();
  g_s.end();

  g_s.begin("Building page list...\n");
  buildPageList(root.get());
  g_s.end();

  g_s.begin("Search for main page...\n");
  findMainPage(root.get());
  findMainPageTagFiles(root.get());
  g_s.end();

  g_s.begin("Computing page relations...\n");
  computePageRelations(root.get());
  checkPageRelations();
  g_s.end();

  g_s.begin("Determining the scope of groups...\n");
  findGroupScope(root.get());
  g_s.end();

  auto memberNameComp = [](const MemberNameLinkedMap::Ptr &n1,const MemberNameLinkedMap::Ptr &n2)
  {
    return qstricmp(n1->memberName()+getPrefixIndex(n1->memberName()),
                    n2->memberName()+getPrefixIndex(n2->memberName())
                   )<0;
  };

  g_s.begin("Sorting lists...\n");
  std::sort(Doxygen::memberNameLinkedMap->begin(),
            Doxygen::memberNameLinkedMap->end(),
            memberNameComp);
  std::sort(Doxygen::functionNameLinkedMap->begin(),
            Doxygen::functionNameLinkedMap->end(),
            memberNameComp);
  Doxygen::hiddenClasses->sort();
  Doxygen::classSDict->sort();
  g_s.end();

  QDir thisDir;
  thisDir.remove(Doxygen::entryDBFileName);

  g_s.begin("Determining which enums are documented\n");
  findDocumentedEnumValues();
  g_s.end();

  g_s.begin("Computing member relations...\n");
  mergeCategories();
  computeMemberRelations();
  g_s.end();

  g_s.begin("Building full member lists recursively...\n");
  buildCompleteMemberLists();
  g_s.end();

  g_s.begin("Adding members to member groups.\n");
  addMembersToMemberGroup();
  g_s.end();

  if (Config_getBool(DISTRIBUTE_GROUP_DOC))
  {
    g_s.begin("Distributing member group documentation.\n");
    distributeMemberGroupDocumentation();
    g_s.end();
  }

  g_s.begin("Computing member references...\n");
  computeMemberReferences();
  g_s.end();

  if (Config_getBool(INHERIT_DOCS))
  {
    g_s.begin("Inheriting documentation...\n");
    inheritDocumentation();
    g_s.end();
  }

  // compute the shortest possible names of all files
  // without losing the uniqueness of the file names.
  g_s.begin("Generating disk names...\n");
  generateDiskNames();
  g_s.end();

  g_s.begin("Adding source references...\n");
  addSourceReferences();
  g_s.end();

  g_s.begin("Adding xrefitems...\n");
  addListReferences();
  generateXRefPages();
  g_s.end();

  g_s.begin("Sorting member lists...\n");
  sortMemberLists();
  g_s.end();

  g_s.begin("Setting anonymous enum type...\n");
  setAnonymousEnumType();
  g_s.end();

  if (Config_getBool(DIRECTORY_GRAPH))
  {
    g_s.begin("Computing dependencies between directories...\n");
    computeDirDependencies();
    g_s.end();
  }

  g_s.begin("Generating citations page...\n");
  CitationManager::instance().generatePage();
  g_s.end();

  g_s.begin("Counting members...\n");
  countMembers();
  g_s.end();

  g_s.begin("Counting data structures...\n");
  countDataStructures();
  g_s.end();

  g_s.begin("Resolving user defined references...\n");
  resolveUserReferences();
  g_s.end();

  g_s.begin("Finding anchors and sections in the documentation...\n");
  findSectionsInDocumentation();
  g_s.end();

  g_s.begin("Transferring function references...\n");
  transferFunctionReferences();
  g_s.end();

  g_s.begin("Combining using relations...\n");
  combineUsingRelations();
  g_s.end();

  g_s.begin("Adding members to index pages...\n");
  addMembersToIndex();
  g_s.end();

  g_s.begin("Correcting members for VHDL...\n");
  vhdlCorrectMemberProperties();
  g_s.end();

  if (Config_getBool(SORT_GROUP_NAMES))
  {
    Doxygen::groupSDict->sort();
    GroupSDict::Iterator gli(*Doxygen::groupSDict);
    GroupDef *gd;
    for (gli.toFirst();(gd=gli.current());++gli)
    {
      gd->sortSubGroups();
    }
  }

}

void generateOutput()
{
  /**************************************************************************
   *            Initialize output generators                                *
   **************************************************************************/

  /// add extra languages for which we can only produce syntax highlighted code
  addCodeOnlyMappings();

  //// dump all symbols
  if (g_dumpSymbolMap)
  {
    dumpSymbolMap();
    exit(0);
  }

  initSearchIndexer();

  bool generateHtml  = Config_getBool(GENERATE_HTML);
  bool generateLatex = Config_getBool(GENERATE_LATEX);
  bool generateMan   = Config_getBool(GENERATE_MAN);
  bool generateRtf   = Config_getBool(GENERATE_RTF);
  bool generateDocbook = Config_getBool(GENERATE_DOCBOOK);


  g_outputList = new OutputList;
  if (generateHtml)
  {
    g_outputList->add<HtmlGenerator>();
    HtmlGenerator::init();

    // add HTML indexers that are enabled
    bool generateHtmlHelp    = Config_getBool(GENERATE_HTMLHELP);
    bool generateEclipseHelp = Config_getBool(GENERATE_ECLIPSEHELP);
    bool generateQhp         = Config_getBool(GENERATE_QHP);
    bool generateTreeView    = Config_getBool(GENERATE_TREEVIEW);
    bool generateDocSet      = Config_getBool(GENERATE_DOCSET);
    if (generateEclipseHelp) Doxygen::indexList->addIndex(new EclipseHelp);
    if (generateHtmlHelp)    Doxygen::indexList->addIndex(new HtmlHelp);
    if (generateQhp)         Doxygen::indexList->addIndex(new Qhp);
    if (generateTreeView)    Doxygen::indexList->addIndex(new FTVHelp(TRUE));
    if (generateDocSet)      Doxygen::indexList->addIndex(new DocSets);
    Doxygen::indexList->initialize();
    HtmlGenerator::writeTabData();
  }
  if (generateLatex)
  {
    g_outputList->add<LatexGenerator>();
    LatexGenerator::init();
  }
  if (generateDocbook)
  {
    g_outputList->add<DocbookGenerator>();
    DocbookGenerator::init();
  }
  if (generateMan)
  {
    g_outputList->add<ManGenerator>();
    ManGenerator::init();
  }
  if (generateRtf)
  {
    g_outputList->add<RTFGenerator>();
    RTFGenerator::init();
  }
  if (Config_getBool(USE_HTAGS))
  {
    Htags::useHtags = TRUE;
    QCString htmldir = Config_getString(HTML_OUTPUT);
    if (!Htags::execute(htmldir))
       err("USE_HTAGS is YES but htags(1) failed. \n");
    else if (!Htags::loadFilemap(htmldir))
       err("htags(1) ended normally but failed to load the filemap. \n");
  }

  /**************************************************************************
   *                        Generate documentation                          *
   **************************************************************************/

  g_s.begin("Generating style sheet...\n");
  //printf("writing style info\n");
  g_outputList->writeStyleInfo(0); // write first part
  g_s.end();

  static bool searchEngine      = Config_getBool(SEARCHENGINE);
  static bool serverBasedSearch = Config_getBool(SERVER_BASED_SEARCH);

  g_s.begin("Generating search indices...\n");
  if (searchEngine && !serverBasedSearch && (generateHtml || g_useOutputTemplate))
  {
    createJavaScriptSearchIndex();
  }

  // generate search indices (need to do this before writing other HTML
  // pages as these contain a drop down menu with options depending on
  // what categories we find in this function.
  if (generateHtml && searchEngine)
  {
    QCString searchDirName = Config_getString(HTML_OUTPUT)+"/search";
    QDir searchDir(searchDirName);
    if (!searchDir.exists() && !searchDir.mkdir(searchDirName))
    {
      term("Could not create search results directory '%s' $PWD='%s'\n",
          searchDirName.data(),QDir::currentDirPath().data());
    }
    HtmlGenerator::writeSearchData(searchDirName);
    if (!serverBasedSearch) // client side search index
    {
      writeJavaScriptSearchIndex();
    }
  }
  g_s.end();

  const FormulaManager &fm = FormulaManager::instance();
  if (fm.hasFormulas() && generateHtml
      && !Config_getBool(USE_MATHJAX))
  {
    g_s.begin("Generating images for formulas in HTML...\n");
    fm.generateImages(Config_getString(HTML_OUTPUT), Config_getEnum(HTML_FORMULA_FORMAT)=="svg" ?
        FormulaManager::Format::Vector : FormulaManager::Format::Bitmap, FormulaManager::HighDPI::On);
    g_s.end();
  }
  if (fm.hasFormulas() && generateRtf)
  {
    g_s.begin("Generating images for formulas in RTF...\n");
    fm.generateImages(Config_getString(RTF_OUTPUT),FormulaManager::Format::Bitmap);
    g_s.end();
  }

  if (fm.hasFormulas() && generateDocbook)
  {
    g_s.begin("Generating images for formulas in Docbook...\n");
    fm.generateImages(Config_getString(DOCBOOK_OUTPUT),FormulaManager::Format::Bitmap);
    g_s.end();
  }

  g_s.begin("Generating example documentation...\n");
  generateExampleDocs();
  g_s.end();

  warn_flush();

  g_s.begin("Generating file sources...\n");
  generateFileSources();
  g_s.end();

  g_s.begin("Generating file documentation...\n");
  generateFileDocs();
  g_s.end();

  g_s.begin("Generating page documentation...\n");
  generatePageDocs();
  g_s.end();

  g_s.begin("Generating group documentation...\n");
  generateGroupDocs();
  g_s.end();

  g_s.begin("Generating class documentation...\n");
  generateClassDocs();
  g_s.end();

  g_s.begin("Generating namespace index...\n");
  generateNamespaceDocs();
  g_s.end();

  if (Config_getBool(GENERATE_LEGEND))
  {
    g_s.begin("Generating graph info page...\n");
    writeGraphInfo(*g_outputList);
    g_s.end();
  }

  g_s.begin("Generating directory documentation...\n");
  generateDirDocs(*g_outputList);
  g_s.end();

  if (g_outputList->size()>0)
  {
    writeIndexHierarchy(*g_outputList);
  }

  g_s.begin("finalizing index lists...\n");
  Doxygen::indexList->finalize();
  g_s.end();

  g_s.begin("writing tag file...\n");
  writeTagFile();
  g_s.end();

  if (Config_getBool(GENERATE_XML))
  {
    g_s.begin("Generating XML output...\n");
    Doxygen::generatingXmlOutput=TRUE;
    generateXML();
    Doxygen::generatingXmlOutput=FALSE;
    g_s.end();
  }
#if USE_SQLITE3
  if (Config_getBool(GENERATE_SQLITE3))
  {
    g_s.begin("Generating SQLITE3 output...\n");
    generateSqlite3();
    g_s.end();
  }
#endif

  if (Config_getBool(GENERATE_AUTOGEN_DEF))
  {
    g_s.begin("Generating AutoGen DEF output...\n");
    generateDEF();
    g_s.end();
  }
  if (Config_getBool(GENERATE_PERLMOD))
  {
    g_s.begin("Generating Perl module output...\n");
    generatePerlMod();
    g_s.end();
  }
  if (generateHtml && searchEngine && serverBasedSearch)
  {
    g_s.begin("Generating search index\n");
    if (Doxygen::searchIndex->kind()==SearchIndexIntf::Internal) // write own search index
    {
      HtmlGenerator::writeSearchPage();
      Doxygen::searchIndex->write(Config_getString(HTML_OUTPUT)+"/search/search.idx");
    }
    else // write data for external search index
    {
      HtmlGenerator::writeExternalSearchPage();
      QCString searchDataFile = Config_getString(SEARCHDATA_FILE);
      if (searchDataFile.isEmpty())
      {
        searchDataFile="searchdata.xml";
      }
      if (!Portable::isAbsolutePath(searchDataFile))
      {
        searchDataFile.prepend(Config_getString(OUTPUT_DIRECTORY)+"/");
      }
      Doxygen::searchIndex->write(searchDataFile);
    }
    g_s.end();
  }

  if (g_useOutputTemplate) generateOutputViaTemplate();

  warn_flush();

  if (generateRtf)
  {
    g_s.begin("Combining RTF output...\n");
    if (!RTFGenerator::preProcessFileInplace(Config_getString(RTF_OUTPUT),"refman.rtf"))
    {
      err("An error occurred during post-processing the RTF files!\n");
    }
    g_s.end();
  }

  warn_flush();

  g_s.begin("Running plantuml with JAVA...\n");
  PlantumlManager::instance()->run();
  g_s.end();

  warn_flush();

  if (Config_getBool(HAVE_DOT))
  {
    g_s.begin("Running dot...\n");
    DotManager::instance()->run();
    g_s.end();
  }

  // copy static stuff
  if (generateHtml)
  {
    FTVHelp::generateTreeViewImages();
    copyStyleSheet();
    copyLogo(Config_getString(HTML_OUTPUT));
    copyExtraFiles(Config_getList(HTML_EXTRA_FILES),"HTML_EXTRA_FILES",Config_getString(HTML_OUTPUT));
  }
  if (generateLatex)
  {
    copyLatexStyleSheet();
    copyLogo(Config_getString(LATEX_OUTPUT));
    copyExtraFiles(Config_getList(LATEX_EXTRA_FILES),"LATEX_EXTRA_FILES",Config_getString(LATEX_OUTPUT));
  }
  if (generateDocbook)
  {
    copyLogo(Config_getString(DOCBOOK_OUTPUT));
  }
  if (generateRtf)
  {
    copyLogo(Config_getString(RTF_OUTPUT));
  }

  if (generateHtml &&
      Config_getBool(GENERATE_HTMLHELP) &&
      !Config_getString(HHC_LOCATION).isEmpty())
  {
    g_s.begin("Running html help compiler...\n");
    QString oldDir = QDir::currentDirPath();
    QDir::setCurrent(Config_getString(HTML_OUTPUT));
    Portable::setShortDir();
    Portable::sysTimerStart();
    if (Portable::system(Config_getString(HHC_LOCATION), "index.hhp", Debug::isFlagSet(Debug::ExtCmd))!=1)
    {
      err("failed to run html help compiler on index.hhp\n");
    }
    Portable::sysTimerStop();
    QDir::setCurrent(oldDir);
    g_s.end();
  }

  warn_flush();

  if ( generateHtml &&
       Config_getBool(GENERATE_QHP) &&
      !Config_getString(QHG_LOCATION).isEmpty())
  {
    g_s.begin("Running qhelpgenerator...\n");
    QCString const qhpFileName = Qhp::getQhpFileName();
    QCString const qchFileName = getQchFileName();

    QCString const args = QCString().sprintf("%s -o \"%s\"", qhpFileName.data(), qchFileName.data());
    QString const oldDir = QDir::currentDirPath();
    QDir::setCurrent(Config_getString(HTML_OUTPUT));
    Portable::sysTimerStart();
    if (Portable::system(Config_getString(QHG_LOCATION), args.data(), FALSE))
    {
      err("failed to run qhelpgenerator on index.qhp\n");
    }
    Portable::sysTimerStop();
    QDir::setCurrent(oldDir);
    g_s.end();
  }

  int cacheParam;
  msg("lookup cache used %zu/%zu hits=%" PRIu64 " misses=%" PRIu64 "\n",
      Doxygen::lookupCache->size(),
      Doxygen::lookupCache->capacity(),
      Doxygen::lookupCache->hits(),
      Doxygen::lookupCache->misses());
  cacheParam = computeIdealCacheParam(Doxygen::lookupCache->misses()*2/3); // part of the cache is flushed, hence the 2/3 correction factor
  if (cacheParam>Config_getInt(LOOKUP_CACHE_SIZE))
  {
    msg("Note: based on cache misses the ideal setting for LOOKUP_CACHE_SIZE is %d at the cost of higher memory usage.\n",cacheParam);
  }

  if (Debug::isFlagSet(Debug::Time))
  {
    msg("Total elapsed time: %.3f seconds\n(of which %.3f seconds waiting for external tools to finish)\n",
         ((double)Debug::elapsedTime()),
         Portable::getSysElapsedTime()
        );
    g_s.print();
  }
  else
  {
    msg("finished...\n");
  }


  /**************************************************************************
   *                        Start cleaning up                               *
   **************************************************************************/

  cleanUpDoxygen();

  finalizeSearchIndexer();
  QDir thisDir;
  thisDir.remove(Doxygen::objDBFileName);
  thisDir.remove(Doxygen::filterDBFileName);
  Config::deinit();
  QTextCodec::deleteAllCodecs();
  delete Doxygen::symbolMap;
  delete Doxygen::clangUsrMap;
  g_successfulRun=TRUE;
}
