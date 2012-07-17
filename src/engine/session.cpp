#include "session.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QMap>

#include "area.h"
#include "characterstats.h"
#include "class.h"
#include "commandevent.h"
#include "commandinterpreter.h"
#include "constants.h"
#include "gameobjectptr.h"
#include "player.h"
#include "race.h"
#include "realm.h"
#include "signinevent.h"
#include "util.h"


class Session::SignUpData {

    public:
        QString userName;
        QString password;

        Race *race;
        Class *characterClass;
        QString gender;

        CharacterStats stats;
        int height;
        int weight;

        SignUpData() :
            race(nullptr),
            characterClass(nullptr),
            height(0),
            weight(0) {
        }
};

Session::Session(Realm *realm, QObject *parent) :
    QObject(parent),
    m_signInStage(SessionClosed),
    m_signUpData(nullptr),
    m_realm(realm),
    m_player(nullptr),
    m_interpreter(nullptr) {
}

Session::~Session() {

    if (m_player && m_player->session() == this) {
        m_player->setSession(nullptr);
    }
    delete m_signUpData;

    delete m_interpreter;
}

void Session::open() {

    write("What is your name? ");
    m_signInStage = AskingUserName;
}

void Session::processSignIn(const QString &data) {

    if (m_signInStage == SignedIn) {
        CommandEvent event(m_interpreter, data);
        event.process();
        return;
    }

    QString input = data.trimmed();
    QString answer = input.toLower();

    QMap<QString, GameObjectPtr> races;
    QMap<QString, GameObjectPtr> classes;
    if (m_signInStage == AskingRace) {
        for (const GameObjectPtr &racePtr : m_realm->races()) {
            if (racePtr.cast<Race *>()->playerSelectable()) {
                races[racePtr->name()] = racePtr;
            }
        }
    } else if (m_signInStage == AskingClass) {
        for (const GameObjectPtr &classPtr : m_signUpData->race->classes()) {
            classes[classPtr->name()] = classPtr;
        }
    }

    bool barbarian = (m_signUpData &&
                      m_signUpData->characterClass &&
                      m_signUpData->characterClass->name() == "Barbarian");

    SignInStage previousSignInStage = m_signInStage;

    switch (m_signInStage) {
        case AskingUserName: {
            QString userName = Util::capitalize(answer.left(12));
            m_player = m_realm->getPlayer(userName);
            if (m_player) {
                m_signInStage = AskingPassword;
            } else {
                m_signUpData = new SignUpData();
                m_signUpData->userName = userName;
                m_signInStage = AskingUserNameConfirmation;
            }
            break;
        }
        case AskingUserNameConfirmation: {
            if (answer == "yes" || answer == "y") {
                m_signInStage = AskingSignupPassword;
            } else if (answer == "no" || answer == "n") {
                m_signInStage = SignInAborted;
            } else {
                write("Please answer with yes or no.\n");
            }
            break;
        }
        case AskingPassword: {
            QByteArray data = QString(m_player->passwordSalt() + input).toUtf8();
            QString passwordHash = QCryptographicHash::hash(data, QCryptographicHash::Sha1)
                                   .toBase64();
            if (m_player->passwordHash() == passwordHash) {
                write(QString("Welcome back, %1. Type %2 if you're feeling lost.\n")
                      .arg(m_player->name(), Util::highlight("help")));
                m_signInStage = SignedIn;
            }  else {
                write("Password incorrect.\n");
            }
            break;
        }
        case AskingSignupPassword:
            if (input.length() < 6) {
                write(Util::colorize("Please choose a password of at least 6 characters.\n", Maroon));
                break;
            }
            if (input.toLower() == m_signUpData->userName.toLower()) {
                write(Util::colorize("Your password and your username may not be the same.\n", Maroon));
                break;
            }
            if (input == "123456" || input == "654321") {
                write(Util::colorize("Sorry, that password is too simple.\n", Maroon));
                break;
            }
            m_signUpData->password = input;
            m_signInStage = AskingSignupPasswordConfirmation;
            break;

        case AskingSignupPasswordConfirmation:
            if (m_signUpData->password == input) {
                write(Util::colorize("Password confirmed.\n", Green));
                m_signInStage = AskingRace;
            } else {
                write(Util::colorize("Passwords don't match.\n", Maroon));
                m_signInStage = AskingSignupPassword;
            }
            break;

        case AskingRace:
            if (races.keys().contains(answer)) {
                write(Util::colorize(QString("\nYou have chosen to become a %1.\n").arg(answer), Green));
                m_signUpData->race = races[answer].cast<Race *>();
                m_signInStage = AskingClass;
            } else if (answer.startsWith("info ")) {
                QString raceName = answer.mid(5).trimmed();
                if (races.keys().contains(raceName)) {
                    write("\n" +
                          Util::highlight(Util::capitalize(raceName)) + "\n"
                          "  " + Util::splitLines(races[raceName].cast<Race *>()->description(), 78).join("\n  ") + "\n");
                } else if (raceName.startsWith("<") && raceName.endsWith(">")) {
                    write(QString("Sorry, you are supposed to replace <race> with the name of an actual race. For example: %1.\n").arg(Util::highlight("info human")));
                } else {
                    write(QString("I don't know anything about the \"%1\" race.\n").arg(raceName));
                }
            }
            break;

        case AskingClass:
            if (classes.keys().contains(answer)) {
                write(Util::colorize(QString("\nYou have chosen to become a %1.\n").arg(answer), Green));
                m_signUpData->characterClass = classes[answer].cast<Class *>();
                m_signInStage = AskingGender;
            } else if (answer.startsWith("info ")) {
                QString className = answer.mid(5).trimmed();
                if (classes.keys().contains(className)) {
                    write("\n" +
                          Util::highlight(Util::capitalize(className)) + "\n"
                          "  " + Util::splitLines(classes[className].cast<Class *>()->description(), 78).join("\n  ") + "\n");
                } else if (className.startsWith("<") && className.endsWith(">")) {
                    write(QString("Sorry, you are supposed to replace <class> with the name of an actual race. For example: %1.\n").arg(Util::highlight("info knight")));
                } else {
                    write(QString("I don't know anything about the \"%1\" class.\n").arg(className));
                }
            } else if (answer == "back" || answer == "b") {
                m_signInStage = AskingRace;
            }
            break;

        case AskingGender:
            if (answer == "male" || answer == "m") {
                write(Util::colorize("\nYou have chosen to be male.\n", Green));
                m_signUpData->gender = "male";
                m_signInStage = AskingExtraStats;
            } else if (answer == "female" || answer == "f") {
                write(Util::colorize("\nYou have chosen to be female.\n", Green));
                m_signUpData->gender = "female";
                m_signInStage = AskingExtraStats;
            } else  if (answer == "back" || answer == "b") {
                m_signInStage = AskingClass;
            }
            break;

        case AskingExtraStats:
            if (answer == "info stats") {
                write(QString("\n"
                              "Your character has several attributes, each of which will have a value assigned. "
                              "Collectively, we call these your character stats. Here is an overview:\n"
                              "\n"
                              "%1 (STR)\n"
                              "  Strength primarily determines the power of your physical attacks. When\n"
                              "  wielding a shield, it also gives a small defense power up.\n"
                              "\n"
                              "%2 (DEX)\n"
                              "  Dexterity determines the speed with which attacks can be dealt. It also \n"
                              "  improves your chances of evading enemy attacks, and the chance of success when\n"
                              "  fleeing.\n"
                              "\n"
                              "%3 (VIT)\n"
                              "  Vitality primarily determines your max. health points (HP).\n"
                              "\n"
                              "%4 (END)\n"
                              "  Endurance primarily determines your physical defense power.\n"
                              "\n"
                              "%5 (INT)\n"
                              "  Intelligence determines your max. magic points (MP).\n"
                              "\n"
                              "%6 (FAI)\n"
                              "  Faith determines the magical defense power. It also decreases the chance that\n"
                              "  a spell will fail when cast.\n")
                      .arg(Util::highlight("Strength"), Util::highlight("Dexterity"),
                           Util::highlight("Vitality"), Util::highlight("Endurance"),
                           Util::highlight("Intelligence"), Util::highlight("Faith")));
            } else if (answer == "back" || answer == "b") {
                m_signInStage = AskingGender;
            } else {
                QStringList attributes = answer.split(' ', QString::SkipEmptyParts);
                if (attributes.length() != (barbarian ? 5 : 6)) {
                    break;
                }

                CharacterStats stats;
                stats.strength = qMax(attributes[0].toInt(), 0);
                stats.dexterity = qMax(attributes[1].toInt(), 0);
                stats.vitality = qMax(attributes[2].toInt(), 0);
                stats.endurance = qMax(attributes[3].toInt(), 0);
                stats.intelligence = barbarian ? 0 : qMax(attributes[4].toInt(), 0);
                stats.faith = qMax(attributes[barbarian ? 4 : 5].toInt(), 0);

                if (stats.total() != 9) {
                    write(Util::colorize("\nThe total of attributes should be 9.\n", Maroon));
                    break;
                }

                m_signUpData->stats += stats;

                m_signUpData->height += stats.intelligence - (stats.dexterity / 2);
                m_signUpData->weight += stats.strength;

                write(Util::colorize("\nYour character stats have been recorded.\n", Green));
                m_signInStage = AskingSignupConfirmation;
            }
            break;

        case AskingSignupConfirmation:
            if (answer == "yes" || answer == "y") {
                QString salt = Util::randomString(8);
                QByteArray data = QString(salt + m_signUpData->password).toUtf8();
                QString hash = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toBase64();

                m_player = GameObject::createByObjectType<Player *>(m_realm, "player");

                m_player->setName(m_signUpData->userName);
                m_player->setPasswordSalt(salt);
                m_player->setPasswordHash(hash);
                m_player->setRace(m_signUpData->race);
                m_player->setClass(m_signUpData->characterClass);
                m_player->setGender(m_signUpData->gender);
                m_player->setStats(m_signUpData->stats);
                m_player->setHeight(m_signUpData->height);
                m_player->setWeight(m_signUpData->weight);
                m_player->setCurrentArea(m_signUpData->race->startingArea());

                m_player->setHp(m_player->maxHp());
                m_player->setMp(m_player->maxMp());
                m_player->setGold(100);

                delete m_signUpData;
                m_signUpData = 0;

                write(QString("\nWelcome to " GAME_TITLE ", %1.\n").arg(m_player->name()));
                m_signInStage = SignedIn;
            } else if (answer == "no" || answer == "n" ||
                       answer == "back" || answer == "b") {
                m_signInStage = AskingExtraStats;
            } else {
                write("Please answer with yes or no.\n");
            }
            break;

        default:
            break;
    }

    if (m_signInStage == AskingRace && races.isEmpty()) {
        for (const GameObjectPtr &racePtr : m_realm->races()) {
            if (racePtr.cast<Race *>()->playerSelectable()) {
                races[racePtr->name()] = racePtr;
            }
        }
    } else if (m_signInStage == AskingClass && classes.isEmpty()) {
        for (const GameObjectPtr &classPtr : m_signUpData->race->classes()) {
            classes[classPtr->name()] = classPtr;
        }
    } else if (m_signInStage == AskingExtraStats) {
        m_signUpData->stats = m_signUpData->race->stats() + m_signUpData->characterClass->stats();
        m_signUpData->height = m_signUpData->race->height();
        m_signUpData->weight = m_signUpData->race->weight();

        if (m_signUpData->characterClass->name() == "knight") {
            m_signUpData->weight += 10;
        }
        if (m_signUpData->characterClass->name() == "warrior" ||
            m_signUpData->characterClass->name() == "soldier" ||
            barbarian) {
            m_signUpData->weight += 5;
        }

        if (barbarian) {
            m_signUpData->stats.intelligence = 0;
        }
        if (m_signUpData->gender == "male") {
            m_signUpData->stats.strength++;
            m_signUpData->height += 10;
            m_signUpData->weight += 10;
        } else {
            m_signUpData->stats.dexterity++;
            m_signUpData->weight -= 10;
        }
    }

    switch (m_signInStage) {
        case AskingUserNameConfirmation:
            write(QString("%1, did I get that right? ").arg(m_signUpData->userName));
            break;

        case AskingPassword:
            write("Please enter your password: ");
            break;

        case AskingSignupPassword:
            write("Please choose a password: ");
            break;

        case AskingSignupPasswordConfirmation:
            write("Please confirm your password: ");
            break;

        case AskingRace:
            if (previousSignInStage != AskingRace) {
                write("\n"
                      "Please select which race you would like your character to be.\n"
                      "Your race determines some attributes of the physique of your character, as "
                      "well as where in the " GAME_TITLE " you will start your journey.\n"
                      "\n"
                      "These are the major races in the " GAME_TITLE ":\n"
                      "\n");
                showColumns(races.keys());
            }
            write(QString("\n"
                          "Please select the race you would like to use, or type %1 to get more information "
                          "about a race.\n")
                  .arg(Util::highlight("info <race>")));
            break;

        case AskingClass:
            if (previousSignInStage != AskingClass) {
                write("\n"
                      "Please select which class you would like your character to be specialized in.\n"
                      "Your class determines additional attributes of the physique of your character, "
                      "and also can influence your choice to be good or evil.\n");
                write(QString("\n"
                              "Note that the available classes are dependent on your choice of race. To "
                              "revisit your choice of race, type %1.\n")
                      .arg(Util::highlight("back")));
                write("These are the classes you may choose from:\n"
                      "\n");
                showColumns(classes.keys());
            }
            write(QString("\n"
                          "Please select the class you would like to use, or type %1 to get more "
                          "information about a class.\n")
                  .arg(Util::highlight("info <class>")));
            break;

        case AskingGender:
            if (previousSignInStage != AskingGender) {
                write("\n"
                      "Please select which gender you would like your character to be.\n"
                      "Your gender has a minor influence on the physique of your character.");
            }
            write(QString("\n"
                          "Please choose %1 or %2.\n"
                          "\n"
                          "To revisit your choice of class, type %3.\n")
                  .arg(Util::highlight("male"),
                       Util::highlight("female"),
                       Util::highlight("back")));
            break;

        case AskingExtraStats:
            if (previousSignInStage != AskingExtraStats) {
                write(QString("\n"
                              "You have selected to become a %1 %2 %3.\n"
                              "Your base character stats are: \n"
                              "\n")
                      .arg(m_signUpData->gender,
                           m_signUpData->race->adjective(),
                           m_signUpData->characterClass->name()) +
                      QString("  %1, %2, %3, %4, %5, %6.\n")
                      .arg(Util::highlight(QString("STR: %1").arg(m_signUpData->stats.strength)),
                           Util::highlight(QString("DEX: %1").arg(m_signUpData->stats.dexterity)),
                           Util::highlight(QString("VIT: %1").arg(m_signUpData->stats.vitality)),
                           Util::highlight(QString("END: %1").arg(m_signUpData->stats.endurance)),
                           Util::highlight(QString("INT: %1").arg(m_signUpData->stats.intelligence)),
                           Util::highlight(QString("FAI: %1").arg(m_signUpData->stats.faith))) +
                      "\n"
                      "You may assign an additional 9 points freely over your various attributes.\n");
            }
            write(QString("\n"
                          "Please enter the distribution you would like to use in the following form:\n"
                          "\n"
                          "  %1\n"
                          "  (Replace every part with a number, for a total of 9. Example: %2)\n"
                          "\n"
                          "To revisit your choice of gender, type %3. If you want more information about "
                          "character stats, type %4.\n")
                  .arg(Util::highlight(barbarian ? "<str> <dex> <vit> <end> <fai>" :
                                                   "<str> <dex> <vit> <end> <int> <fai>"),
                       Util::highlight(barbarian ? "2 2 2 2 1" : "2 2 2 1 1 1"),
                       Util::highlight("back"),
                       Util::highlight("info stats")));
            break;

        case AskingSignupConfirmation:
            if (previousSignInStage != AskingSignupConfirmation) {
                write(QString("\n"
                              "You have selected to become a %1 %2 %3.\n"
                              "Your final character stats are: \n"
                              "\n")
                      .arg(m_signUpData->gender,
                           m_signUpData->race->adjective(),
                           m_signUpData->characterClass->name()) +
                      QString("  %1, %2, %3, %4, %5, %6.\n")
                      .arg(Util::highlight(QString("STR: %1").arg(m_signUpData->stats.strength)),
                           Util::highlight(QString("DEX: %1").arg(m_signUpData->stats.dexterity)),
                           Util::highlight(QString("VIT: %1").arg(m_signUpData->stats.vitality)),
                           Util::highlight(QString("END: %1").arg(m_signUpData->stats.endurance)),
                           Util::highlight(QString("INT: %1").arg(m_signUpData->stats.intelligence)),
                           Util::highlight(QString("FAI: %1").arg(m_signUpData->stats.faith))));
            }
            write("\n"
                  "Are you ready to create a character with these stats?\n");
            break;

        case SignedIn:
            if (m_player->session()) {
                write("Cannot sign you in because you're already signed in from another location.");
                terminate();
                break;
            }
            m_player->setSession(this);
            connect(m_player, SIGNAL(write(QString)), this, SIGNAL(write(QString)));

            m_interpreter = new CommandInterpreter(m_player);
            connect(m_interpreter, SIGNAL(quit()), this, SIGNAL(terminate()));
            m_player->enter(m_player->currentArea());
            break;

        case SignInAborted:
            write("Ok. Bye.");
            terminate();
            break;

        default:
            break;
    }
}

void Session::onUserInput(QString data) {

    if (m_signInStage == SessionClosed) {
        qDebug() << "User input on closed session";
        return;
    }

    if (data.isEmpty()) {
        return;
    }

    if (!m_player || !m_player->isAdmin()) {
        data = data.left(160);
    }

    if (m_signInStage == SignedIn && m_interpreter) {
        m_realm->enqueueEvent(new CommandEvent(m_interpreter, data));
    } else {
        m_realm->enqueueEvent(new SignInEvent(this, data));
    }
}

void Session::showColumns(const QStringList &items) {

    int length = items.length();
    int halfLength = length / 2 + length % 2;
    for (int i = 0; i < halfLength; i++) {
        QString first = Util::capitalize(items[i]);
        QString second = Util::capitalize(i + halfLength < length ? items[i + halfLength] : "");

        write("  " + Util::highlight(first.leftJustified(30)) +
              "  " + Util::highlight(second));
    }
}