#include "commandregistry.h"

#include <QRegExp>
#include <QScriptValue>
#include <QStringList>

#include "room.h"
#include "exit.h"
#include "gameexception.h"
#include "gameobjectptr.h"
#include "logutil.h"
#include "player.h"
#include "util.h"
#include "commands/buycommand.h"
#include "commands/closecommand.h"
#include "commands/descriptioncommand.h"
#include "commands/disbandcommand.h"
#include "commands/dropcommand.h"
#include "commands/eatcommand.h"
#include "commands/equipmentcommand.h"
#include "commands/followcommand.h"
#include "commands/getcommand.h"
#include "commands/givecommand.h"
#include "commands/gocommand.h"
#include "commands/groupcommand.h"
#include "commands/gtalkcommand.h"
#include "commands/helpcommand.h"
#include "commands/inventorycommand.h"
#include "commands/killcommand.h"
#include "commands/lookcommand.h"
#include "commands/losecommand.h"
#include "commands/opencommand.h"
#include "commands/quitcommand.h"
#include "commands/removecommand.h"
#include "commands/saycommand.h"
#include "commands/scriptcommand.h"
#include "commands/searchcommand.h"
#include "commands/shoutcommand.h"
#include "commands/slashmecommand.h"
#include "commands/statscommand.h"
#include "commands/talkcommand.h"
#include "commands/tellcommand.h"
#include "commands/usecommand.h"
#include "commands/wieldcommand.h"
#include "commands/whocommand.h"
#include "commands/admin/addcharactercommand.h"
#include "commands/admin/addcontainercommand.h"
#include "commands/admin/addexitcommand.h"
#include "commands/admin/additemcommand.h"
#include "commands/admin/addshieldcommand.h"
#include "commands/admin/addweaponcommand.h"
#include "commands/admin/copyitemcommand.h"
#include "commands/admin/copytriggerscommand.h"
#include "commands/admin/execscriptcommand.h"
#include "commands/admin/getpropcommand.h"
#include "commands/admin/gettriggercommand.h"
#include "commands/admin/listmethodscommand.h"
#include "commands/admin/listpropscommand.h"
#include "commands/admin/removeexitcommand.h"
#include "commands/admin/removeitemcommand.h"
#include "commands/admin/setclasscommand.h"
#include "commands/admin/setpropcommand.h"
#include "commands/admin/setracecommand.h"
#include "commands/admin/settriggercommand.h"
#include "commands/admin/stopservercommand.h"
#include "commands/admin/unsettriggercommand.h"
#include "commands/api/datagetcommand.h"
#include "commands/api/datasetcommand.h"
#include "commands/api/exitsetcommand.h"
#include "commands/api/exitslistcommand.h"
#include "commands/api/logretrievecommand.h"
#include "commands/api/propertygetcommand.h"
#include "commands/api/propertysetcommand.h"
#include "commands/api/roomslistcommand.h"
#include "commands/api/triggergetcommand.h"
#include "commands/api/triggersetcommand.h"
#include "commands/api/triggerslistcommand.h"


CommandRegistry::CommandRegistry(QObject *parent) :
    QObject(parent) {

    Command *get = new GetCommand(this);
    Command *go = new GoCommand(this);
    Command *kill = new KillCommand(this);
    Command *look = new LookCommand(this);
    Command *quit = new QuitCommand(this);

    m_commands.insert("attack", kill);
    m_commands.insert("buy", new BuyCommand(this));
    m_commands.insert("close", new CloseCommand(this));
    m_commands.insert("description", new DescriptionCommand(this));
    m_commands.insert("disband", new DisbandCommand(this));
    m_commands.insert("drop", new DropCommand(this));
    m_commands.insert("eat", new EatCommand(this));
    m_commands.insert("enter", go);
    m_commands.insert("equipment", new EquipmentCommand(this));
    m_commands.insert("examine", look);
    m_commands.insert("follow", new FollowCommand(this));
    m_commands.insert("get", get);
    m_commands.insert("give", new GiveCommand(this));
    m_commands.insert("go", go);
    m_commands.insert("goodbye", quit);
    m_commands.insert("group", new GroupCommand(this));
    m_commands.insert("gtalk", new GtalkCommand(this));
    m_commands.insert("help", new HelpCommand(this));
    m_commands.insert("inventory", new InventoryCommand(this));
    m_commands.insert("kill", kill);
    m_commands.insert("l", look);
    m_commands.insert("look", look);
    m_commands.insert("lose", new LoseCommand(this));
    m_commands.insert("open", new OpenCommand(this));
    m_commands.insert("quit", quit);
    m_commands.insert("remove", new RemoveCommand(this));
    m_commands.insert("say", new SayCommand(this));
    m_commands.insert("search", new SearchCommand(this));
    m_commands.insert("shout", new ShoutCommand(this));
    m_commands.insert("stats", new StatsCommand(this));
    m_commands.insert("take", get);
    m_commands.insert("talk", new TalkCommand(this));
    m_commands.insert("tell", new TellCommand(this));
    m_commands.insert("use", new UseCommand(this));
    m_commands.insert("wield", new WieldCommand(this));
    m_commands.insert("who", new WhoCommand(this));
    m_commands.insert("/me", new SlashMeCommand(this));

    m_adminCommands.insert("add-character", new AddCharacterCommand(this));
    m_adminCommands.insert("add-container", new AddContainerCommand(this));
    m_adminCommands.insert("add-exit", new AddExitCommand(this));
    m_adminCommands.insert("add-item", new AddItemCommand(this));
    m_adminCommands.insert("add-shield", new AddShieldCommand(this));
    m_adminCommands.insert("add-weapon", new AddWeaponCommand(this));
    m_adminCommands.insert("copy-item", new CopyItemCommand(this));
    m_adminCommands.insert("copy-triggers", new CopyTriggersCommand(this));
    m_adminCommands.insert("exec-script", new ExecScriptCommand(this));
    m_adminCommands.insert("get-prop", new GetPropCommand(this));
    m_adminCommands.insert("get-trigger", new GetTriggerCommand(this));
    m_adminCommands.insert("list-methods", new ListMethodsCommand(this));
    m_adminCommands.insert("list-props", new ListPropsCommand(this));
    m_adminCommands.insert("remove-exit", new RemoveExitCommand(this));
    m_adminCommands.insert("remove-item", new RemoveItemCommand(this));
    m_adminCommands.insert("set-class", new SetClassCommand(this));
    m_adminCommands.insert("set-prop", new SetPropCommand(this));
    m_adminCommands.insert("set-race", new SetRaceCommand(this));
    m_adminCommands.insert("set-trigger", new SetTriggerCommand(this));
    m_adminCommands.insert("stop-server", new StopServerCommand(this));
    m_adminCommands.insert("unset-trigger", new UnsetTriggerCommand(this));

    m_apiCommands.insert("api-data-get", new DataGetCommand(this));
    m_apiCommands.insert("api-data-set", new DataSetCommand(this));
    m_apiCommands.insert("api-exit-set", new ExitSetCommand(this));
    m_apiCommands.insert("api-exits-list", new ExitsListCommand(this));
    m_apiCommands.insert("api-log-retrieve", new LogRetrieveCommand(this));
    m_apiCommands.insert("api-property-get", new PropertyGetCommand(this));
    m_apiCommands.insert("api-property-set", new PropertySetCommand(this));
    m_apiCommands.insert("api-rooms-list", new RoomsListCommand(this));
    m_apiCommands.insert("api-trigger-get", new TriggerGetCommand(this));
    m_apiCommands.insert("api-trigger-set", new TriggerSetCommand(this));
    m_apiCommands.insert("api-triggers-list", new TriggersListCommand(this));
}

CommandRegistry::~CommandRegistry() {
}

void CommandRegistry::registerCommand(const QString &commandName, Command *command) {

    if (m_commands.contains(commandName)) {
        delete m_commands[commandName];
    }

    m_commands[commandName] = command;
}

void CommandRegistry::registerCommand(const QString &commandName, const QScriptValue &object) {

    Command *command = new ScriptCommand(object, this);
    registerCommand(commandName, command);
}

bool CommandRegistry::contains(const QString &commandName) const {

    return m_commands.contains(commandName);
}

Command *CommandRegistry::command(const QString &commandName) const {

    return m_commands[commandName];
}

QString CommandRegistry::description(const QString &commandName) const {

    if (m_commands.contains(commandName)) {
        return m_commands[commandName]->description();
    } else {
        return QString();
    }
}

bool CommandRegistry::adminCommandsContains(const QString &commandName) const {

    return m_adminCommands.contains(commandName);
}

Command *CommandRegistry::adminCommand(const QString &commandName) const {

    return m_adminCommands[commandName];
}

QString CommandRegistry::adminCommandDescription(const QString &commandName) const {

    if (m_adminCommands.contains(commandName)) {
        return m_adminCommands[commandName]->description();
    } else {
        return QString();
    }
}

bool CommandRegistry::apiCommandsContains(const QString &commandName) const {

    return m_apiCommands.contains(commandName);
}

Command *CommandRegistry::apiCommand(const QString &commandName) const {

    return m_apiCommands[commandName];
}

QString CommandRegistry::apiCommandDescription(const QString &commandName) const {

    if (m_apiCommands.contains(commandName)) {
        return m_apiCommands[commandName]->description();
    } else {
        return QString();
    }
}