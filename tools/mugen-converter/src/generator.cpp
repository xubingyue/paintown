#include "generator.h"

#include "ast/all.h"
#include "util/regex.h"

#include <ctime>
#include <exception>
#include <list>

using namespace Mugen;

std::string getCurrentDate(){
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80];

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    
    strftime (buffer,80,"%A - %m/%d/%Y - %I:%M%p.",timeinfo);
    
    return std::string(buffer);
}

static int toLower( int c ){ return tolower( c );}

const std::string lowercase( const std::string &str ){
    std::string tempStr = str;
    transform( tempStr.begin(), tempStr.end(), tempStr.begin(), toLower );
    return tempStr;
}

/* Example on how to use the PythonStream */
void testStream(const std::string & file){
    Mugen::PythonStream out;
    out.open(file);
    out << "import mugen" << PythonStream::endl;
    out <<  "class TestCharacter(mugen.Character):" << PythonStream::endl;
        out << PythonStream::indent;
        out << "def __init__(self):" << PythonStream::endl;
            out << PythonStream::indent;
            out << "self.setName('TestCharacter')" << PythonStream::endl;
    out.clearIndents();
    out << "char = TestCharacter()" << PythonStream::endl;
    out << "print char.name" << PythonStream::endl;
    out.close();
}

/* Simplify use */
PythonStream::IndentType indent = PythonStream::indent;
PythonStream::IndentType unindent = PythonStream::unindent;
StreamEnd endl = PythonStream::endl;

// Content
Content::Content():
totalLines(0){
}
Content::Content(unsigned int level, const std::string & str):
totalLines(1){
    indentLevel.push_back(level);
    content.push_back(str);
}

Content::Content(const Content & copy):
totalLines(0){
    totalLines = copy.totalLines;
    indentLevel = copy.indentLevel;
    content = copy.content;
}
Content::~Content(){
}

const Content & Content::operator=(const Content & copy){
    totalLines = copy.totalLines;
    indentLevel = copy.indentLevel;
    content = copy.content;
    
    return *this;
}

void Content::addLine(unsigned int level, const std::string & str){
    indentLevel.push_back(level);
    content.push_back(str);
    totalLines++;
}

void Content::addSpace(){
    indentLevel.push_back(0);
    content.push_back("");
    totalLines++;
}

void Content::output(PythonStream & stream, unsigned int indentStart){
    for (unsigned int i = 0; i < totalLines; ++i){
        stream.setIndentLevel(indentStart + indentLevel[i]);
        stream << content[i] << endl;
    }
}

const unsigned int Content::getIndentLevel() const{
    if (totalLines!=0){
        return indentLevel[0];
    }
    return 0;
}


// Python definition
PythonDefinition::PythonDefinition(const Content & content):
defLine(content){
}
PythonDefinition::PythonDefinition(const PythonDefinition & copy){
    defLine = copy.defLine;
    content = copy.content;
}
PythonDefinition::~PythonDefinition(){
}
const PythonDefinition & PythonDefinition::operator=(const PythonDefinition & copy){
    defLine = copy.defLine;
    content = copy.content;
    return *this;
}
void PythonDefinition::addContent(const Content & cont){
    content.push_back(cont);
}
void PythonDefinition::addSpace(){
    content.push_back(Content(0,""));
}
void PythonDefinition::output(PythonStream & stream, unsigned int indentStart){
    defLine.output(stream, indentStart);
    stream.setIndentLevel(indentStart + defLine.getIndentLevel());
    for (std::vector<Content>::iterator i = content.begin(); i != content.end(); ++i){
        Content & cont = *i;
        cont.output(stream, indentStart + defLine.getIndentLevel());
    }
}

// Python class
PythonClass::PythonClass(const Content & content):
classLine(content),
init(Content(1,"def __init__(self):")){
    init.addSpace();
    init.addContent(Content(1,"# Initialize"));
    init.addContent(Content(1,"mugen.Character.__init__(self)"));
}
PythonClass::PythonClass(const PythonClass & copy):
init(Content(1,"def __init__(self):")){
    classLine = copy.classLine;
    init = copy.init;
    definitions = definitions;
}
PythonClass::~PythonClass(){
}
const PythonClass & PythonClass::operator=(const PythonClass & copy){
    classLine = copy.classLine;
    init = copy.init;
    definitions = definitions;
    return *this;
}
void PythonClass::add(const PythonDefinition & def){
    definitions.push_back(def);
}
void PythonClass::output(PythonStream & stream, unsigned int indentStart){
    classLine.output(stream, indentStart);
    stream.setIndentLevel(indentStart + classLine.getIndentLevel());
    init.output(stream, indentStart + classLine.getIndentLevel());
    for (std::vector<PythonDefinition>::iterator i = definitions.begin(); i != definitions.end(); ++i){
        PythonDefinition & def = *i;
        def.output(stream, indentStart + classLine.getIndentLevel());
    }
}


CharacterGenerator::CharacterGenerator(const std::string & filename):
filename(filename),
directory(Mugen::stripFilename(filename)){
}

CharacterGenerator::~CharacterGenerator(){
}

void CharacterGenerator::output(const std::string & file){
    try {
        std::cout << "Preparing character definition file " << filename << " for output to file: " << file << std::endl;
        
        Mugen::PythonStream out;
        out.open(file);;
        
        out << "# File generated from original file '" << Mugen::stripDir(filename) << "' on " << getCurrentDate() << endl;
        out << endl;
        
        out << "import mugen" << endl;
        out << endl;
        // Create character class
        PythonClass character(Content(0, "class " + Mugen::stripExtension(file) + "(mugen.Character):"));
        handleBaseDef(character);
        handleCmdFile(character);
        handleStateFiles(character);
        character.output(out, 0);
        out.close();
    } catch (const Mugen::Cmd::ParseException & fail){
        std::cout << "Failed to parse " << filename << " because " << fail.getReason() << std::endl;
    } catch (...){
        std::cout << "Unknown Exception Caught!\nAborting..." << std::endl;
    }
}

void CharacterGenerator::addStateFile(const std::string & stateFile){
    // Don't add the same file twice
    for (std::vector<std::string>::iterator i = stateFiles.begin(); i != stateFiles.end(); ++i){
        if (stateFile == *i){
            // File already added, don't save
            return;
        }
    }
    stateFiles.push_back(stateFile);
}

void CharacterGenerator::handleBaseDef(PythonClass & character){
    class BaseWalker: public Ast::Walker {
        public:
            BaseWalker(CharacterGenerator & generator, PythonClass & character):
            generator(generator),
            character(character){
                
            }
            
            CharacterGenerator & generator;
            PythonClass & character;
            std::string sectionName;
            Content content;
            
            virtual void onSection(const Ast::Section & section){
                sectionName = section.getName();
                std::cout << "Parsing Section: " << sectionName << std::endl;
            }
            virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                if (simple == "name"){
                    try{
                        addComment(simple.getLine());
                        content.addLine(1,"self.name = '" + simple.valueAsString() + "'");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "displayname"){
                    try{
                        addComment(simple.getLine());
                        content.addLine(1,"self.displayName = '" + simple.valueAsString() + "'");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "versiondate"){
                    try{
                        std::vector<int> numbers;
                        simple >> numbers;
                        std::stringstream out;
                        out << "self.versionDate = [";
                        unsigned int count = 0;
                        for (std::vector<int>::iterator it = numbers.begin(); it != numbers.end(); it++){
                            out << *it << (count != numbers.size()-1 ? "," : "");
                            count++;
                        }
                        out << "]";
                        addComment(simple.getLine());
                        content.addLine(1,out.str());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "mugenversion"){
                    try{
                        std::vector<int> numbers;
                        simple >> numbers;
                        std::stringstream out;
                        out << "self.mugenVersion = [";
                        unsigned int count = 0;
                        for (std::vector<int>::iterator it = numbers.begin(); it != numbers.end(); it++){
                            out << *it << (count != numbers.size()-1 ? "," : "");
                            count++;
                        }
                        out << "]";
                        addComment(simple.getLine());
                        content.addLine(1, out.str());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "author"){
                    try{
                        addComment(simple.getLine());
                        content.addLine(1,"self.author = '" + simple.valueAsString() + "'");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "pal.defaults"){
                        std::vector<int> numbers;
                        simple >> numbers;
                        std::stringstream out;
                        out << "self.paletteDefaults = [";
                        unsigned int count = 0;
                        for (std::vector<int>::iterator it = numbers.begin(); it != numbers.end(); it++){
                            out << *it << (count != numbers.size()-1 ? "," : "");
                            count++;
                        }
                        out << "]";
                        addComment(simple.getLine());
                        content.addLine(1,out.str());
                } else if (simple == "cmd"){
                    try{
                        simple >> generator.commandFile;
                        addComment(simple.getLine());
                        content.addLine(1,"self.cmdFile = '" + generator.commandFile + "'");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "cns"){
                    try{
                        generator.addStateFile(simple.valueAsString());
                        addComment(simple.getLine());
                        content.addLine(1,"self.addStateFile('" + generator.stateFiles.back() + "')");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "st"){
                    try{
                        generator.addStateFile(simple.valueAsString());
                        addComment(simple.getLine());
                        content.addLine(1,"self.addStateFile('" + generator.stateFiles.back() + "')");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "stcommon"){
                    try{
                        generator.addStateFile(simple.valueAsString());
                        addComment(simple.getLine());
                        content.addLine(1,"self.addStateFile('" + generator.stateFiles.back() + "')");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (Util::matchRegex(simple.idString(), "st[0-9]+")){
                    try{
                        generator.addStateFile(simple.valueAsString());
                        addComment(simple.getLine());
                        content.addLine(1,"self.addStateFile('" + generator.stateFiles.back() + "')");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "sprite"){
                    try{
                        addComment(simple.getLine());
                        content.addLine(1,"self.spriteFile = '" + simple.valueAsString() + "'");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "anim"){
                    try{
                        simple >> generator.airFile;
                        addComment(simple.getLine());
                        content.addLine(1,"self.animFile = '" + generator.airFile + "'");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "sound"){
                    try{
                        addComment(simple.getLine());
                        content.addLine(1,"self.soundFile = '" + simple.valueAsString() + "'");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (Util::matchRegex(simple.idString(), "pal[0-9]+")){
                    std::string id = simple.idString();
                    try{
                        addComment(simple.getLine());
                        content.addLine(1,"self.paletteFiles[" + id.substr(3) + "] = '" + simple.valueAsString() + "'");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "intro.storyboard"){
                    try{
                        addComment(simple.getLine());
                        content.addLine(1,"self.introStoryboard = '" + simple.valueAsString() + "'");
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "ending.storyboard"){
                    try{
                        addComment(simple.getLine());
                        content.addLine(1,"self.endingStoryboard = '" + simple.valueAsString() + "'");
                    } catch (const Ast::Exception & fail){
                    }
                } else {
                    std::cout << "Unhandled option in " << sectionName << " Section: " << simple.toString() << std::endl;
                }
            }
            
            virtual void onAttributeKeyword (const Ast::AttributeKeyword &simple){
                std::cout << "Attribute keyword: " << simple.toString() << std::endl;
            }
            
            virtual void onAttributeArray (const Ast::AttributeArray &simple){
                std::cout << "Attribute array: " << simple.toString() << std::endl;
            }

            virtual void onNumber (const Ast::Number &simple){
                std::cout << "number: " << simple.toString() << std::endl;
            }
            
            virtual void complete (){
                // Add content
                character.getInit().addContent(content);
            }
            
            virtual void addComment (int line){
                std::stringstream out;
                out << line;
                content.addSpace();
                content.addLine(1,"# [" + Mugen::stripDir(generator.filename) + "] Section - " + sectionName + " on line " + out.str());
            }
    };
    
    BaseWalker walker(*this, character);
    std::list<Ast::Section*> * sections = parseDef(filename);
    for (std::list<Ast::Section*>::iterator it = sections->begin(); it != sections->end(); it++){
        Ast::Section * section = *it;
        section->walk(walker);
        std::list<Ast::Attribute*>  attributes = section->getAttributes();
    }
    destroy(sections);
    walker.complete();
}

class StateException : public std::exception{
    virtual const char* what() const throw()
    {
        return "";
    }
};

class StateHandler{
    public:
        StateHandler(const std::string & section):
        inDef(true){
            if (Util::matchRegex(lowercase(section), "statedef")){
                sectionName = section;
                std::string func = "state";
                if (sectionName[10] == '-'){
                    // Found negative, it's either -3, -2 or -1 (but check and store anyways)
                    std::string num = Util::captureRegex(lowercase(sectionName), "statedef -([0-9]+)", 0);
                    stateNumber = "-" + num;
                    func += "neg" + num;
                } else {
                    std::string num = Util::captureRegex(lowercase(sectionName), "statedef ([0-9]+)", 0);
                    stateNumber = num;
                    func += num;
                }
                
                function = func;
                definition = "def " + function + "():";
            } else throw StateException();
        }
        ~StateHandler(){
        }
        
        std::string sectionName;
        std::string definition;
        std::string function;
        std::string stateNumber;
        std::string statedefParameters;
        std::string initComment;
        bool inDef;
        
        void toggleSection(){
            inDef = !inDef;
        }
        
        void parseSection(const std::string & filename, const Ast::AttributeSimple & simple){
            // In Statedef
            if (inDef){
                handleDef(filename, simple);
            } else {
                handleState(filename, simple);
            }
        }
        
        void handleDef(const std::string & filename, const Ast::AttributeSimple & simple){
            /*  type
                    movetype
                    physics
                    anim
                    velset
                    ctrl
                    poweradd
                    juggle
                    facep2
                    hitdefpersist
                    movehitpersist
                    hitcountpersist
                    sprpriority */
            if (simple == "type"){
            } else if (simple == "movetype"){
            } else if (simple == "physics"){
            } else if (simple == "anim"){
            } else if (simple == "velset"){
            } else if (simple == "ctrl"){
            } else if (simple == "poweradd"){
            } else if (simple == "juggle"){
            } else if (simple == "facep2"){
            } else if (simple == "hitdefpersist"){
            } else if (simple == "movehitpersist"){
            } else if (simple == "hitcounterpersist"){
            } else if (simple == "sprpriority"){
            } else {
                std::cout << "Unhandled option in " << sectionName << " Section: " << simple.toString() << std::endl;
            }
        }
        
        void handleState(const std::string & filename, const Ast::AttributeSimple & simple){
            if (simple == "triggerall"){
            } else if (Util::matchRegex(simple.idString(), "trigger[0-9]+")){
            } else if (Util::matchRegex(simple.idString(), "var([0-9]+)")){
            } if (simple == "persistent"){
            } else {
                std::cout << "Unhandled option in " << sectionName << " Section: " << simple.toString() << std::endl;
            }
        }
        
        void makeInitComment(const std::string & file, int line){
                std::stringstream out;
                out << line;
                initComment = "# [" + Mugen::stripDir(file) + "] Section - " + sectionName + " on line " + out.str();
        }
        
        const Content getInitEntry(){
            Content content;
            content.addSpace();
            content.addLine(1, initComment);
            content.addLine(1, "self.addState(" + stateNumber + ", mugen.StateDef(" + function + ", " + statedefParameters + "))");
            return content;
        }
};

void CharacterGenerator::handleCmdFile(PythonClass & character){
    
    class Command{
        public:
            Command(){
            }
            std::string name;
            std::string command;
            std::string time;
            std::string bufferTime;
            
            std::string get(){
                return "self.addCommand('" + name + "', '" + command + "', " + (time.empty() ? "0" : time) + ", " + (bufferTime.empty() ? "0" : bufferTime) + ")";
            }
    };
        
    class CmdWalker: public Ast::Walker {
        public:
            CmdWalker(CharacterGenerator & generator, PythonClass & character ):
            generator(generator),
            character(character),
            currentCommand(NULL){
                
            }
            
            ~CmdWalker(){
                if (currentCommand != NULL){
                    delete currentCommand;
                }
            }
            
            CharacterGenerator & generator;
            PythonClass & character;
            std::string sectionName;
            
            Content constantsContent;
            
            Command * currentCommand;
            Content commandContent;
            
            std::vector<StateHandler *> states;
            
            virtual void onSection(const Ast::Section & section){
                sectionName = section.getName();
                
                if (lowercase(sectionName) == "command"){
                    // Create new command class to put away and store the previous one
                    if (currentCommand == NULL){
                        currentCommand = new Command();
                    } else if (currentCommand != NULL){
                        // Add command
                        commandContent.addLine(1,currentCommand->get());
                        delete currentCommand;
                        currentCommand = new Command();
                    }
                } else if (Util::matchRegex(lowercase(sectionName), "state")){
                    // TODO Create state handler
                }
                std::cout << "Parsing Section: " << section.getName() << std::endl;   
            }
            virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                if (simple == "command.time"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.commandTime = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "command.buffer.time"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.commandBufferTime = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "name"){
                    if (currentCommand != NULL){
                        addCommandComment(simple.getLine());
                        currentCommand->name = simple.valueAsString();
                    }
                } else if (simple == "command"){
                    if (currentCommand != NULL){
                        std::string command;
                        try {
                            std::string cmd;
                            simple >> cmd;
                            command += cmd + ",";
                        } catch (Ast::Exception &ex){
                        }
                        currentCommand->command = command.substr(0,command.size()-1);
                    }
                } else if (simple == "time"){
                    if (currentCommand != NULL){
                        currentCommand->time = simple.valueAsString();
                    }
                } else if (simple == "buffer.time"){
                    if (currentCommand != NULL){
                        currentCommand->bufferTime = simple.valueAsString();
                    }
                } else if (simple == "type"){
                    // TODO add to state
                } else if (simple == "value"){
                    // TODO add to state
                } else if (simple == "triggerall"){
                    // TODO add to state
                } else if (Util::matchRegex(simple.idString(), "trigger[0-9]+")){
                    // TODO add to state
                } else if (Util::matchRegex(simple.idString(), "var([0-9]+)")){
                    // TODO add to state
                } else {
                    std::cout << "Unhandled option in " << sectionName << " Section: " << simple.toString() << std::endl;
                }
            }
            
            virtual void onAttributeKeyword (const Ast::AttributeKeyword &simple){
                std::cout << "Attribute keyword: " << simple.toString() << std::endl;
            }
            
            virtual void onAttributeArray (const Ast::AttributeArray &simple){
                std::cout << "Attribute array: " << simple.toString() << std::endl;
            }

            virtual void onNumber (const Ast::Number &simple){
                std::cout << "number: " << simple.toString() << std::endl;
            }
            
            virtual void addConstantsComment (int line){
                std::stringstream out;
                out << line;
                constantsContent.addSpace();
                constantsContent.addLine(1,"# [" + Mugen::stripDir(generator.commandFile) + "] Section - " + sectionName + " on line " + out.str());
            }
            
            virtual void addCommandComment (int line){
                std::stringstream out;
                out << line;
                commandContent.addSpace();
                commandContent.addLine(1,"# [" + Mugen::stripDir(generator.commandFile) + "] Section - " + sectionName + " on line " + out.str());
            }
            
            virtual void complete (){
                // Add constants content
                character.getInit().addContent(constantsContent);
                
                // Add command content
                commandContent.addLine(1,currentCommand->get());
                character.getInit().addContent(commandContent);
            }
    };
    
    // Command Walker
    CmdWalker cmd(*this, character);
    std::list<Ast::Section*> * sections = parseCmd(directory + commandFile);
    for (std::list<Ast::Section*>::iterator it = sections->begin(); it != sections->end(); ++it){
        Ast::Section * section = *it;
        section->walk(cmd);
        std::list<Ast::Attribute*>  attributes = section->getAttributes();
    }
    destroy(sections);
    cmd.complete();
}

/* Handle cns files
 * TODO need to add statedef/state information into class */
void CharacterGenerator::handleStateFiles(PythonClass & character){
    class StateWalker: public Ast::Walker {
        public:
            StateWalker(CharacterGenerator & generator, PythonClass & character, const std::string & currentFile ):
            generator(generator),
            character(character),
            currentFile(currentFile){
                
            }
            
            CharacterGenerator & generator;
            PythonClass & character;
            const std::string & currentFile;
            std::string sectionName;
            
            Content constantsContent;
            
            virtual void onSection(const Ast::Section & section){
                sectionName = section.getName();
                
                std::cout << "Parsing Section: " << section.getName() << std::endl;   
            }
            virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                if (simple == "life"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.life = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "attack"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.attack = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "defence"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.defence = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "fall.defence_up"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.fallDefenceUp = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "liedown.time"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.liedownTime = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "airjuggle"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.airjuggle = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "sparkno" && !Util::matchRegex(sectionName, "State")){
                    // Do not include state
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.sparkno = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "guard.sparkno"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.guardSparkno = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "KO.echo"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.koEcho = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "volume" && !Util::matchRegex(sectionName, "State")){
                    // Do not include state
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.volume = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "IntPersistIndex"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.intPersistIndex = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "FloatPersistIndex"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.floatPersistIndex = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "xscale"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.xscale = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "yscale"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.yscale = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "ground.back"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.groundBack = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "ground.front"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.groundFront = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "air.back"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.airBack = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "air.front"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.airFront = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "height"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.height = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "attack.dist"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.attackDist = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "proj.attack.dist"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.projAttackDist = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "proj.doscale"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.projDoscale = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "head.pos"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.headPos.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.headPos.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "mid.pos"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.midPos.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.midPos.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "shadowoffset"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.shadowoffset = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "draw.offset"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.drawOffset.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.drawOffset.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "walk.fwd"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.walkFwd.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.walkFwd.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "walk.back"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.walkBack.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.walkBack.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "run.fwd"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.runFwd.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.runFwd.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "run.back"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.runBack.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.runBack.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "jump.neu"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.jumpNeu.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.jumpNeu.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "jump.back"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.jumpBack.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.jumpBack.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "jump.fwd"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.jumpFwd.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.jumpFwd.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "runjump.back"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.runjumpBack.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.runjumpBack.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "runjump.fwd"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.runjumpFwd.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.runjumpFwd.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "airjump.neu"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.airjumpNeu.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.airjumpNeu.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "airjump.back"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.airjumpBack.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.airjumpBack.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "airjump.fwd"){
                    int x=0, y=0;
                    try{
                        simple >> x >> y;
                    } catch (const Ast::Exception & fail){
                    }
                    std::stringstream out;
                    addConstantsComment(simple.getLine());
                    out << "self.airjumpFwd.append(" << x << ")";
                    constantsContent.addLine(1,out.str());
                    out.str("");
                    out << "self.airjumpFwd.append(" << y << ")";
                    constantsContent.addLine(1,out.str());
                } else if (simple == "airjump.num"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.airjumpNum = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "airjump.height"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.airjumpHeight = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "yaccel" && !Util::matchRegex(sectionName, "State")){
                    // Do not include state
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.yaccel = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "stand.friction"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.standFriction = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else if (simple == "crouch.friction"){
                    try{
                        addConstantsComment(simple.getLine());
                        constantsContent.addLine(1,"self.crouchFriction = " + simple.valueAsString());
                    } catch (const Ast::Exception & fail){
                    }
                } else {
                    std::cout << "Unhandled option in " << sectionName << " Section: " << simple.toString() << std::endl;
                }
            }
            
            virtual void onAttributeKeyword (const Ast::AttributeKeyword &simple){
                std::cout << "Attribute keyword: " << simple.toString() << std::endl;
            }
            
            virtual void onAttributeArray (const Ast::AttributeArray &simple){
                std::cout << "Attribute array: " << simple.toString() << std::endl;
            }

            virtual void onNumber (const Ast::Number &simple){
                std::cout << "number: " << simple.toString() << std::endl;
            }
            
            virtual void addConstantsComment (int line){
                std::stringstream out;
                out << line;
                constantsContent.addSpace();
                constantsContent.addLine(1,"# [" + Mugen::stripDir(currentFile) + "] Section - " + sectionName + " on line " + out.str());
            }
            
            virtual void complete (){
                // Add constants content
                character.getInit().addContent(constantsContent);
            }
    };
    
    for (std::vector<std::string>::iterator i = stateFiles.begin(); i != stateFiles.end(); ++i){
        StateWalker state(*this, character, *i);
        std::list<Ast::Section*> * sections = parseCmd(directory + *i);
        for (std::list<Ast::Section*>::iterator it = sections->begin(); it != sections->end(); it++){
            Ast::Section * section = *it;
            section->walk(state);
            std::list<Ast::Attribute*>  attributes = section->getAttributes();
        }
        destroy(sections);
        state.complete();
    }
    
}