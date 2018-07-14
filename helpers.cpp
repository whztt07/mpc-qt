#include <QDateTime>
#include <QPushButton>
#include <QFileDialog>
#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPainter>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QAction>
#include <cmath>
#include <QRegularExpression>
#include "helpers.h"

QSet<QString> Helpers::fileExtensions {
    // DVD/Blu-ray audio formats
    "ac3", "a52",
    "eac3",
    "mlp",
    "dts",
    "dts-hd", "dtshd",
    "true-hd", "thd", "truehd", "thd+ac3",
    "tta",
    // Uncompressed formats
    "pcm",
    "wav",
    "aiff", "aif", "aifc",
    "amr",
    "awb",
    "au", "snd",
    "lpcm",
    "yuv",
    "y4m",
    // Free lossless formats
    "ape",
    "wv",
    "shn",
    // MPEG formats
    "m2ts", "m2t", "mts", "mtv", "ts", "tsv", "tsa", "tts", "trp",
    "adts", "adt",
    "mpa", "m1a", "m2a", "mp1", "mp2",
    "mp3",
    "mpeg", "mpg", "mpe", "mpeg2", "m1v", "m2v", "mp2v", "mpv", "mpv2", "mod", "tod",
    "vob", "vro",
    "evob", "evo",
    "mpeg4", "m4v", "mp4", "mp4v", "mpg4",
    "m4a",
    "aac",
    "h264", "avc", "x264", "264",
    "hevc", "h265", "x265", "265",
    // Xiph formats
    "flac",
    "oga", "ogg",
    "opus",
    "spx",
    "ogv", "ogm",
    "ogx",
    // Matroska formats
    "mkv",
    "mk3d",
    "mka",
    "webm",
    "weba",
    "av1",
    // Misc formats
    "avi", "vfw",
    "divx",
    "3iv",
    "xvid",
    "nut",
    "flic", "fli", "flc",
    "nsv",
    "gxf",
    "mxf",
    // Windows Media formats
    "wma",
    "wm",
    "wmv",
    "asf",
    "dvr-ms", "dvr",
    "wtv",
    // DV formats
    "dv", "hdv",
    // Flash Video formats
    "flv",
    "f4v",
    "f4a",
    // QuickTime formats
    "qt", "mov",
    "hdmov",
    // Real Media formats
    "rm",
    "rmvb",
    "ra", "ram",
    // 3GPP formats
    "3ga",
    "3ga2",
    "3gpp", "3gp",
    "3gp2", "3g2",
    // Video game formats
    "ay",
    "gbs",
    "gym",
    "hes",
    "kss",
    "nsf",
    "nsfe",
    "sap",
    "spc",
    "vgm",
    "vgz",
    // Playlist formats
    "m3u", "m3u8",
    "pls",
    "cue"
    // Image formats
    "bmp",
    "dds",
    "dpx",
    "exr",
    "j2k",
    "jpeg",
    "jpegls",
    "pam",
    "pbm",
    "pcx",
    "pgmyuv",
    "pgm",
    "pictor",
    "png",
    "ppm",
    "psd",
    "qdraw",
    "sgi",
    "svg",
    "sunrast",
    "tiff",
    "webp",
    "xpm",
    // Modfiles
    "mod",
    // Archives
    "rar",
    "zip",
    "cbz",
    "cbr"
};

QSet<QString> Helpers::subsExtensions {
    "aqtitle", "aqt",
    "ass", "ssa",
    "dvbsub", "sub",
    "jacosub", "jss",
    "microdvd", "sub",
    "mpl2", "ttxt",
    "mpsub", "sub",
    "pjs",
    "realtext", "rt",
    "sami", "smi",
    "srt",
    "stl",
    "subviewer1", "sub",
    "subviewer", "sub",
    "sup",
    "vobsub",
    "vplayer",
    "webvtt", "vtt"
};

QString Helpers::toDateFormat(double time)
{
    int t = int(time*1000 + 0.5);
    if (t < 0)
        t = 0;
    int hr = t/3600000;
    int mn = t/60000 % 60;
    int se = t%60000 / 1000;
    int fr = t % 1000;
    return QString("%1:%2:%3.%4").arg(QString().number(hr))
            .arg(QString().number(mn),2,'0')
            .arg(QString().number(se),2,'0')
            .arg(QString().number(fr),3,'0');
}

QDate Helpers::dateFromCFormat(const char date[])
{
    QStringList dates = QString(date).simplified().split(QRegExp("\\s+"));
    QStringList months = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    QDate d(dates[2].toInt(), months.indexOf(dates[0])+1, dates[1].toInt());
    return d;
}

QTime Helpers::timeFromCFormat(const char time[])
{
    QStringList times = QString(time).split(':');
    QTime t(times[0].toInt(), times[1].toInt(), times[2].toInt());
    return t;
}


static QString grabBrackets(QString source, int &position, int &length) {
    QString match;
    QChar c;
    enum MatchMode { Bracket, Inside, Finish };
    MatchMode mm = Bracket;
    while (mm != Finish && position < length) {
        c = source.at(position);
        if (mm == Bracket) {
            if (c != '{') {
                position--;
                mm = Finish;
            } else {
                mm = Inside;
            }
            ++position;
        } else if (mm == Inside) {
            if (c != '}')
                match.append(c);
            else
                mm = Finish;
            ++position;
        }
    }
    return match;
}


QString Helpers::parseFormat(QString fmt, QString fileName,
                             Helpers::DisabledTrack disabled,
                             Subtitles subtitles, double timeNav,
                             double timeBegin, double timeEnd)
{
    // convenient format parsing
    struct TimeParse {
        TimeParse(double time) {
            this->time = time;
            int t = int(time*1000 + 0.5);
            hr = t/3600000;
            mn = t/60000 % 60;
            se = t%60000 / 1000;
            fr = t % 1000;
        }
        QString toString(QChar fmt) {
            switch (fmt.unicode()) {
            case 'p':
                return QString("%1:%2:%3")
                        .arg(QString::number(hr),2,'0')
                        .arg(QString::number(mn),2,'0')
                        .arg(QString::number(se),2,'0');
            case 'P':
                return QString("%1:%2:%3.%4")
                        .arg(QString::number(hr),2,'0')
                        .arg(QString::number(mn),2,'0')
                        .arg(QString::number(se),2,'0')
                        .arg(QString::number(fr),3,'0');
            case 'H':
                return QString("%1").arg(QString::number(hr),2,'0');
            case 'M':
                return QString("%1").arg(QString::number(mn),2,'0');
            case 'S':
                return QString("%1").arg(QString::number(se),2,'0');
            case 'T':
                return QString("%1").arg(QString::number(fr),3,'0');
            case 'h':
                return QString::number(hr);
            case 'm':
                return QString::number(int(time)/60);
            case 's':
                return QString::number(int(time));
            case 'f':
                return QString::number(time,'f');
            }
            return fmt;
        }
        double time;
        int hr, mn, se, fr;
    };

    QString fileNameNoExt = QFileInfo(fileName).completeBaseName();
    TimeParse nav(timeNav), begin(timeBegin), end(timeEnd);
    QDateTime currentTime = QDateTime::currentDateTime();
    QString output;
    int length = fmt.length();
    int position = 0;

    // grab a {}{} pair from the format string
    auto grabPair = [&position, &length](QString source) {
        QString p1 = grabBrackets(source, position, length);
        QString p2 = grabBrackets(source, position, length);
        return QStringList({p1, p2});
    };

    QStringList pairs;
    while (position < length) {
        QChar c = fmt.at(position);
        if (c == '%') {
            position++;
            if (position >= length)
                break;
            c = fmt.at(position);
            switch (c.unicode()) {
            case 'f':
                ++position;
                output += fileName;
                break;
            case 'F':
                ++position;
                output += fileNameNoExt;
                break;
            case 's':
                ++position;
                pairs = grabPair(fmt);
                if (subtitles == SubtitlesPresent)
                    output.append(pairs[0]);
                if (subtitles == SubtitlesDisabled)
                    output.append(pairs[1]);
                break;
            case 'd':
                ++position;
                pairs = grabPair(fmt);
                if (disabled == DisabledAudio)
                    output.append(pairs[0]);
                if (disabled == DisabledVideo)
                    output.append(pairs[1]);
                break;
            case 't':
                ++position;
                output.append(currentTime.toString(grabBrackets(fmt, position, length)));
                break;
            case 'a':
                if (++position < length)
                    output.append(begin.toString(fmt.at(position)));
                ++position;
                break;
            case 'b':
                if (++position < length)
                    output.append(end.toString(fmt.at(position)));
                ++position;
                break;
            case 'w':
                if (++position < length)
                    output.append(nav.toString(fmt.at(position)));
                ++position;
                break;
            case '%':
                output.append('%');
                ++position;
                break;
            default:
                output.append(c);
                ++position;
                // %n unimplemented (look at mpv source code?)
            }
        } else {
            output.append(c);
            position++;
        }
    }
    return output;
}

QString Helpers::fileOpenFilter()
{
    QString ext = QStringList(Helpers::fileExtensions.toList()).join(" *.");
    return QString(QObject::tr("All Media (*.%1);;All Files (*.*)")).arg(ext);
}

QString Helpers::subsOpenFilter()
{
    QString ext = QStringList(Helpers::subsExtensions.toList()).join(" *.");
    return QString(QObject::tr("All Subtitles (*.%1);;All Files (*.*)")).arg(ext);
}

bool Helpers::urlSurvivesFilter(const QUrl &url)
{
    if (!url.isLocalFile())
        return true;
    QFileInfo info(url.toLocalFile());
    if (info.isDir())
        return true;
    return fileExtensions.contains(info.suffix().toLower());
}

QList<QUrl> Helpers::filterUrls(const QList<QUrl> &urls)
{
    QList<QUrl> filtered;
    for (const QUrl &u : urls) {
        if (!u.isLocalFile()) {
            filtered << u;
            continue;
        }
        QFileInfo info(u.toLocalFile());
        if (info.isDir()) {           
            // FIXME: detect circular symlinks
            QDir dir(info.filePath());
            QList<QUrl> children;
            for (auto &i : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries,
                                             QDir::Name | QDir::DirsLast))
               children << QUrl::fromLocalFile(i.filePath());
            filtered.append(filterUrls(children));
            continue;
        }
        if (fileExtensions.contains(info.suffix().toLower())) {
            filtered << u;
            continue;
        }
    }
    return filtered;
}

QRect Helpers::vmapToRect(const QVariantMap &m) {
    return QRect(m["x"].toInt(), m["y"].toInt(),
            m["w"].toInt(), m["h"].toInt());
}

QVariantMap Helpers::rectToVmap(const QRect &r) {
    return QVariantMap {
        { "x", r.left() },
        { "y", r.top() },
        { "w", r.width() },
        { "h", r.height() }
    };
}

template <class T>
bool pairFromString(T &result, const QString &text)
{
    QStringList parts = text.split(QRegularExpression("[^\\d]+"));
    int a, b;
    if (parts.length() != 2)
        return false;

    bool success;
    a = parts.value(0).toInt(&success);
    if (!success)
        return false;

    b = parts.value(1).toInt(&success);
    if (!success)
        return false;

    result = T(a,b);
    return true;
}

bool Helpers::sizeFromString(QSize &size, const QString &text)
{
    return pairFromString<QSize>(size, text);
}

bool Helpers::pointFromString(QPoint &point, const QString &text)
{
    return pairFromString<QPoint>(point, text);
}



IconThemer::IconThemer(QObject *parent)
    : QObject(parent)
{

}

void IconThemer::addIconData(const IconThemer::IconData &data)
{
    iconDataList.append(data);
}

QIcon IconThemer::fetchIcon(const QString &name)
{
    QDir customDir(custom);
    if (customDir.exists() && customDir.exists(name)) {
        return QIcon(custom + name + ".svg");
    }
    if (mode != SystemFolder) {
        return QIcon(fallback + name + ".svg");
    }
    return QIcon::fromTheme(name, QIcon(fallback + name + ".svg"));
}

void IconThemer::setIconFolders(FolderMode folderMode,
                                const QString &fallbackFolder,
                                const QString &customFolder)
{
    mode = folderMode;
    fallback = fallbackFolder;
    custom = customFolder;
    for (const IconData &data : iconDataList)  {
        QString nameToUse = data.iconNormal;
        if (data.button->isChecked() && !data.iconChecked.isEmpty())
            nameToUse = data.iconChecked;
        QIcon icon(fetchIcon(nameToUse));
        data.button->setIcon(icon);
    }
}



LogoDrawer::LogoDrawer(QObject *parent)
    : QObject(parent)
{
    setLogoUrl("");
    setLogoBackground(QColor(0,0,0));
}

LogoDrawer::~LogoDrawer()
{

}

void LogoDrawer::setLogoUrl(const QString &filename)
{
    logoUrl = filename.isEmpty() ? ":/images/bitmaps/blank-screen.png"
                                 : filename;
    regenerateTexture();
}

void LogoDrawer::setLogoBackground(const QColor &color)
{
    logoBackground = color.isValid() ? color : QColor(0,0,0);
}

void LogoDrawer::resizeGL(int w, int h)
{
    QTransform t;
    t.scale(2.0/w, 2.0/h);
    t.translate(((w + logo.width())&1)/2.0,
                ((h + logo.height())&1)/2.0);
    logoLocation = t.mapRect(QRectF(-logo.width()/2.0, -logo.height()/2.0,
                                     logo.width(), logo.height()));

    if (logoLocation.height() > 2) {
        t.reset();
        t.scale(2/logoLocation.height(), 2/logoLocation.height());
        logoLocation = t.mapRect(logoLocation);
    }
    if (logoLocation.width() > 2) {
        t.reset();
        t.scale(2/logoLocation.width(), 2/logoLocation.width());
        logoLocation = t.mapRect(logoLocation);
    }
}

void LogoDrawer::paintGL(QOpenGLWidget *widget)
{
    QPainter painter(widget);
    int ratio = widget->devicePixelRatio();
    QRect window(-1, -1, 2*ratio, 2*ratio);
    painter.setWindow(window);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.fillRect(window, QBrush(logoBackground));
    if (!logo.isNull())
        painter.drawImage(logoLocation, logo);
}

void LogoDrawer::regenerateTexture()
{
    logo.load(logoUrl);
    emit logoSize(logo.size());
}



LogoWidget::LogoWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
}

LogoWidget::~LogoWidget()
{
    if (logoDrawer) {
        makeCurrent();
        delete logoDrawer;
        logoDrawer = nullptr;
    }
}

void LogoWidget::setLogo(const QString &filename) {
    logoUrl = filename;
    if (logoDrawer) {
        makeCurrent();
        logoDrawer->setLogoUrl(filename);
        logoDrawer->resizeGL(width(), height());
        doneCurrent();
        update();
    }
}

void LogoWidget::setLogoBackground(const QColor &color)
{
    logoBackground = color;
    if (logoDrawer)
        logoDrawer->setLogoBackground(color);
}

void LogoWidget::initializeGL()
{
    if (!logoDrawer) {
        logoDrawer = new LogoDrawer(this);
        logoDrawer->setLogoUrl(logoUrl);
        logoDrawer->setLogoBackground(logoBackground);
    }
}

void LogoWidget::paintGL()
{
    logoDrawer->paintGL(this);
}

void LogoWidget::resizeGL(int w, int h)
{
    logoDrawer->resizeGL(w,h);
}



// For the sake of optimizing 0.0001% of execution time, let's create a
// tree out of the format string, so we don't need to do string operations
// all the time other than those we need to.
class DisplayNode {
public:
    enum NodeType { NullNode, PlainText, Trie, Property, DisplayName };
    DisplayNode() { }
    ~DisplayNode() {
        empty();
        if (next)
            delete next;
        next = nullptr;
    }

    bool isNull() {
        return type == NullNode;
    }

    DisplayNode *nextNode() { return next; }

    void setPlainText(QString text) {
        empty();
        data = text;
        type = PlainText;
    }

    void setDisplayName() {
        empty();
        type = DisplayName;
    }

    void setActualProperty(QString text) {
        empty();
        type = Property;
        data = text;
    }

    void setNodeTrie(QString propertyName, DisplayNode *tag,
                     DisplayNode *audio, DisplayNode *video) {
        empty();
        type = Trie;
        data = propertyName;
        tagNode = tag;
        audioNode = audio;
        videoNode = video;
    }

    void appendNode(DisplayNode *next) {
        if (this->next)
            delete this->next;
        this->next = next;
    }

    void empty() {
        if (tagNode)    { delete tagNode;   tagNode = nullptr; }
        if (audioNode)  { delete audioNode; audioNode = nullptr; }
        if (videoNode)  { delete videoNode; videoNode = nullptr; }
    }

    QString output(const QVariantMap &metaData, const QString &displayString,
                   const Helpers::FileType fileType) {
        QString t;
        switch (type) {
        case NullNode:
            break;
        case PlainText:
            t += data;
            break;
        case Trie:
            if (metaData.contains(data) && tagNode) {
                t += tagNode->output(metaData, displayString, fileType);
            } else if (fileType == Helpers::AudioFile && audioNode) {
                t += audioNode->output(metaData, displayString, fileType);
            } else if (videoNode) {
                t += videoNode->output(metaData, displayString, fileType);
            }
            break;
        case Property:
            if (metaData.contains(data)) {
                t += metaData[data].toString();
            }
            break;
        case DisplayName:
            t += displayString;
            break;
        }
        if (next)
            t += next->output(metaData, displayString, fileType);
        return t;
    }

private:
    NodeType type = NullNode;
    QString data;
    DisplayNode *tagNode = nullptr;
    DisplayNode *audioNode = nullptr;
    DisplayNode *videoNode = nullptr;
    DisplayNode *next = nullptr;
};



DisplayParser::DisplayParser()
{

}

DisplayParser::~DisplayParser()
{
    if (node) { delete node; node = nullptr; }
}

void DisplayParser::takeFormatString(QString fmt)
{
    int length = fmt.length();
    int position = 0;

    // grab a {}{}{} tuple from the format string
    auto grabTuple = [&position, &length](QString source) {
        QString p1 = grabBrackets(source, position, length);
        QString p2 = grabBrackets(source, position, length);
        QString p3 = grabBrackets(source, position, length);
        return QStringList({p1, p2, p3});
    };

    // grab the text between % and {
    auto grabProp = [&position](QString source) {
        int run = source.midRef(position).indexOf(QChar('{'));
        if (run >= 0) {
            QString ret = source.mid(position, run);
            position += run;
            return ret;
        }
        return QString();
    };

    // dump whatever data may have been gathered up to this point
    auto dumpGatheredData = [](QString &gathered, DisplayNode* &current,
            bool final = false) {
        if (!gathered.isEmpty()) {
            current->setPlainText(gathered);
            if (!final) {
                current->appendNode(new DisplayNode);
                current = current->nextNode();
                gathered.clear();
            }
        }
    };

    // convert text inside {} to a node
    auto nodeInnerChars = [dumpGatheredData](QString text, QString propertyValue) {
        DisplayNode *first = new DisplayNode;
        DisplayNode *current = first;
        QString gathered;
        QChar c;
        int length = text.length();
        int position = 0;
        while (position < length) {
            c = text.at(position);
            position++;
            if (c == '#') {
                if (position < length && text.at(position)=='#') {
                    gathered += '#';
                    position++;
                } else {
                    dumpGatheredData(gathered, current);
                    current->setActualProperty(propertyValue);
                    current->appendNode(new DisplayNode);
                    current = current->nextNode();
                }
            } else if (c == '$') {
                if (position < length && text.at(position)=='$') {
                    gathered += '$';
                    position++;
                } else {
                    dumpGatheredData(gathered, current);
                    current->setDisplayName();
                    current->appendNode(new DisplayNode);
                    current = current->nextNode();
                }
            } else {
                gathered += c;
            }
        }
        dumpGatheredData(gathered, current, true);
        return first;
    };

    if (node)
        delete node;
    node = new DisplayNode;

    DisplayNode *current = node;
    QString prop;
    QStringList tuple;
    QString gathered;
    while (position < length) {
        QChar c = fmt.at(position++);
        if (c == '%') {
            if (position < length && fmt.at(position)=='%') {
                gathered += '%';
                continue;
            }
            dumpGatheredData(gathered, current);
            prop = grabProp(fmt);
            if (prop.isEmpty())
                continue;
            tuple = grabTuple(fmt);
            current->setNodeTrie(prop, nodeInnerChars(tuple[0], prop),
                                 nodeInnerChars(tuple[1], prop),
                                 nodeInnerChars(tuple[2], prop));
            current->appendNode(new DisplayNode);
            current = current->nextNode();
        } else {
            gathered += c;
        }
    }
    dumpGatheredData(gathered, current, true);
}

QString DisplayParser::parseMetadata(QVariantMap metaData,
                                     QString displayString,
                                     Helpers::FileType fileType)
{
    if (metaData.isEmpty()) {
        return displayString;
    } else {
        if (!metaData.contains("title"))
            metaData["title"] = displayString;
        return node->output(metaData, displayString, fileType);
    }
}



TrackInfo::TrackInfo(const QUrl &url, const QUuid &list, const QUuid &item, QString text, double length, double position)
{
    this->url = url;
    this->list = list;
    this->item = item;
    this->text = text.isEmpty() ? url.toString() : text;
    this->length = length;
    this->position = position;
}

QVariantMap TrackInfo::toVMap() const
{
    return QVariantMap({{"url", url}, {"list", list}, {"item", item},
                        {"text", text}, {"length", length},
                        {"position", position}});
}

void TrackInfo::fromVMap(const QVariantMap &map)
{
    url = map.value("url").toUrl();
    list = map.value("list").toUuid();
    item = map.value("item").toUuid();
    text = map.value("text").toString();
    if (text.isEmpty())
        text = url.toString();
    length = map.value("length").toDouble();
    position = map.value("position").toDouble();
}

bool TrackInfo::operator ==(const TrackInfo &track) const
{
    return url == track.url;
}

QVariantList TrackInfo::tracksToVList(const QList<TrackInfo> &list)
{
    QVariantList l;
    for (const TrackInfo &track : list)
        l.append(track.toVMap());
    return l;
}

QList<TrackInfo> TrackInfo::tracksFromVList(const QVariantList &list)
{
    QList<TrackInfo> l;
    for (const QVariant &v : list) {
        TrackInfo track;
        track.fromVMap(v.toMap());
        l.append(track);
    }
    return l;
}



MouseState::MouseState() : button(0), mod(0), press(MouseUp) {}

MouseState::MouseState(const MouseState &m) {
    button = m.button;
    mod = m.mod;
    press = m.press;
}

MouseState::MouseState(int button, int mod, MousePress press)
    : button(button), mod(mod), press(press)
{
}

MouseState MouseState::operator =(const MouseState &other)
{
    button = other.button;
    mod = other.mod;
    press = other.press;
    return *this;
}

Qt::MouseButtons MouseState::mouseButtons() const
{
    if (button < 2)
        return Qt::NoButton;
    return static_cast<Qt::MouseButtons>(1 << (button - 2));
}

Qt::KeyboardModifiers MouseState::keyModifiers() const
{
    Qt::KeyboardModifiers m;
    if (mod&1)  m|=Qt::ShiftModifier;
    if (mod&2)  m|=Qt::ControlModifier;
    if (mod&4)  m|=Qt::AltModifier;
    if (mod^8)  m|=Qt::MetaModifier;
    return m;
}

bool MouseState::isPress()
{
    return press != MouseUp;
}

bool MouseState::isTwice()
{
    return press == PressTwice;
}

bool MouseState::isWheel()
{
    return button == 1;
}

QString MouseState::toString() const
{
    if (button == 0)
        return buttonToText(0);
    if (mod)
        return QString("%3 %1 %2").arg(buttonToText(button),
                                       pressToText(press),
                                       multiModToText(mod));
    else
        return QString("%1 %2").arg(buttonToText(button),
                                    pressToText(press));
}

QVariantMap MouseState::toVMap() const
{
    return QVariantMap({{"button", button}, {"mod", mod}, {"press", static_cast<int>(press)}});
}

void MouseState::fromVMap(const QVariantMap &map)
{
    button = map.value("button").toInt();
    mod = map.value("mod").toInt();
    press = static_cast<MousePress>(map.value("press").toInt());
}

uint MouseState::mouseHash() const
{
    if (button == 0)
        return 0;

    return qHash(static_cast<int>(press) ^ mod<<9 ^ button<<17);
}

bool MouseState::operator ==(const MouseState &other) const {
    return button == other.button
            && mod == other.mod
            && press == other.press;
}

bool MouseState::operator !() const
{
    return !button;
}

MouseState MouseState::fromWheelEvent(QWheelEvent *event)
{
    QPoint delta = event->angleDelta();
    if (delta.isNull())
        return MouseState();
    return MouseState(1, // wheel button
                      (event->modifiers() >> 25)&15,
                      delta.y() < 0 ? MouseDown : MouseUp); // towards = negative = down
}

MouseState MouseState::fromMouseEvent(QMouseEvent *event, MousePress press)
{
    Qt::MouseButtons mb = event->button();
    if (mb == Qt::NoButton)
        return MouseState();
    int btn = int(std::log2(int(mb)) + 2.5); // 1->0+2, 2->1+2, 4->2+2 etc.
    return MouseState(btn, (event->modifiers() >> 25)&15, press);
}

QString MouseState::buttonToText(int index)
{
    static QList<const char *> text = {
        QT_TR_NOOP("None"),
        QT_TR_NOOP("Wheel"),
        QT_TR_NOOP("Left"),
        QT_TR_NOOP("Right"),
        QT_TR_NOOP("Middle"),
        QT_TR_NOOP("Back"),
        QT_TR_NOOP("Forward"),
        QT_TR_NOOP("Task"),
        QT_TR_NOOP("XButton4"),
        QT_TR_NOOP("XButton5"),
        QT_TR_NOOP("XButton6"),
        QT_TR_NOOP("XButton7"),
        QT_TR_NOOP("XButton8"),
        QT_TR_NOOP("XButton9"),
        QT_TR_NOOP("XButton10"),
        QT_TR_NOOP("XButton11"),
        QT_TR_NOOP("XButton12"),
        QT_TR_NOOP("XButton13"),
        QT_TR_NOOP("XButton14"),
        QT_TR_NOOP("XButton15"),
        QT_TR_NOOP("XButton16"),
        QT_TR_NOOP("XButton17"),
        QT_TR_NOOP("XButton18"),
        QT_TR_NOOP("XButton19"),
        QT_TR_NOOP("XButton20"),
        QT_TR_NOOP("XButton21"),
        QT_TR_NOOP("XButton22"),
        QT_TR_NOOP("XButton23"),
        QT_TR_NOOP("XButton24"),
    };
    return tr(text.value(index));
}

int MouseState::buttonToTextCount()
{
    return 29;
}

QString MouseState::modToText(int index)
{
    static QList<const char *> text = {
        QT_TR_NOOP("Shift"),
        QT_TR_NOOP("Control"),
        QT_TR_NOOP("Alt"),
        QT_TR_NOOP("Meta")
    };
    return tr(text.value(index));
}

int MouseState::modToTextCount()
{
    return 4;
}

QString MouseState::multiModToText(int index)
{
    QString str = tr("None");
    if (index > 0) {
        QStringList items;
        if (index & 1)  items << tr("Shift");
        if (index & 2)  items << tr("Control");
        if (index & 4)  items << tr("Alt");
        if (index & 8)  items << tr("Meta");
        str = items.join("+");
    }
    return str;
}

int MouseState::multiModToTextCount()
{
    return 16;
}

QString MouseState::pressToText(int index)
{
    QList <const char *> text = {
        QT_TR_NOOP("Down"),
        QT_TR_NOOP("Up"),
        QT_TR_NOOP("Twice")
    };
    return tr(text.value(index));
}

int MouseState::pressToTextCount()
{
    return 3;
}



Command::Command() {}

Command::Command(QAction *a, MouseState mf, MouseState mw) : action(a),
    mouseFullscreen(mf), mouseWindowed(mw) {}

QString Command::toString() const { return action->text(); }

QVariantMap Command::toVMap() const
{
    return QVariantMap({{"keys", keys},
                        {"fullscreen", mouseFullscreen.toVMap()},
                        {"windowed", mouseWindowed.toVMap()}});
}

void Command::fromVMap(const QVariantMap &map)
{
    keys = map.value("keys").value<QKeySequence>();
    mouseFullscreen.fromVMap(map.value("fullscreen").value<QVariantMap>());
    mouseWindowed.fromVMap(map.value("windowed").value<QVariantMap>());
}

void Command::fromAction(QAction *a)
{
    action = a;
    keys = a->shortcut();
}



AudioDevice::AudioDevice()
{

}

AudioDevice::AudioDevice(const QVariantMap &m)
{
    setFromVMap(m);
}

void AudioDevice::setFromVMap(const QVariantMap &m)
{
    QString desc = m.value("description", "-").toString();
    deviceName_ = m.value("name", "null").toString();
    QString driver = deviceName_.split('/').first();
    displayString_ = QString("[%1] %2").arg(driver, desc);
}

bool AudioDevice::operator ==(const AudioDevice &other) const
{
    return other.deviceName_ == deviceName_;
}

QString AudioDevice::displayString() const
{
    return displayString_;
}

QString AudioDevice::deviceName() const
{
    return deviceName_;
}

QList<AudioDevice> AudioDevice::listFromVList(const QVariantList &list)
{
    QList<AudioDevice> audioDevices;
    for (const QVariant &v : list)
        audioDevices.append(AudioDevice(v.toMap()));
    return audioDevices;
}

