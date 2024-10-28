#include "widget.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMouseEvent>
#include <QApplication>
#include <QScreen>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <mutex>

const std::vector<QString> PriceWidget::defaultSymbols = {"BTC", "ETH", "SOL"};

PriceWidget::PriceWidget(const std::vector<QString>& symbols, QWidget* parent)
    : QWidget(parent)
    , symbols(symbols)
    , networkManager(std::make_unique<QNetworkAccessManager>())
{
    // Set window flags for frameless, always-on-top, and tool-like behavior
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    // Enable transparent background
    setAttribute(Qt::WA_TranslucentBackground);
    // Set window type to dock
    setAttribute(Qt::WA_X11NetWmWindowTypeDock, true);
    
    // Connect the finished signal of the network manager to the handleNetworkReply slot
    // This ensures that the handleNetworkReply function is called when a network request is finished
    connect(networkManager.get(), &QNetworkAccessManager::finished,
            this, &PriceWidget::handleNetworkReply);
    
    initUI();
}

void PriceWidget::initUI() {
    layout = new QVBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(10, 10, 10, 10);

    for (const auto& symbol : symbols) {
        auto* label = new QLabel("Loading...", this);
        label->setStyleSheet("color: rgb(0, 255, 0)");
        label->setFont(QFont("Monospace", 12));
        layout->addWidget(label);
        priceLabels[symbol] = label;
    }

    updateTimer = new QTimer(this);
    // Connect the timer's timeout signal to the updatePrices slot
    // When the timer times out, the updatePrices function will be called
    connect(updateTimer, &QTimer::timeout, this, &PriceWidget::updatePrices);
    updateTimer->start(120000); 

    updatePrices();
}

void PriceWidget::updatePrices() {
    for (const auto& symbol : symbols) {
        QString url = apiUrl + symbol + "USDT";
        QNetworkRequest request(url);
        networkManager->get(request);
    }
}

void PriceWidget::handleNetworkReply(QNetworkReply* reply) {
    reply->deleteLater(); // RAII cleanup

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Network error:" << reply->errorString();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QString symbol = obj["symbol"].toString();
        QString price = obj["price"].toString();
        
        // Remove USDT suffix for display
        symbol.remove("USDT");
        
        // Remove trailing zeros and decimal point if needed
        while (price.endsWith('0')) price.chop(1);
        if (price.endsWith('.')) price.chop(1);

        {
            std::unique_lock<std::shared_mutex> lock(priceMutex);
            // Compare with previous price for trend
            QString trend;
            if (priceCache.contains(symbol)) {
                double oldPrice = priceCache[symbol].toDouble();
                double newPrice = price.toDouble();
                trend = (newPrice > oldPrice) ? " ↑" : 
                       (newPrice < oldPrice) ? " ↓" : " -";
            } else {
                trend = " -";
            }
            priceCache[symbol] = price;
            updateLabel(symbol, price + trend);
        }
    }
}

void PriceWidget::updateLabel(const QString& symbol, const QString& price) {
    if (auto* label = priceLabels.value(symbol)) {
        label->setText(QString("%1/USDT: %2").arg(symbol, price));
    }
}

void PriceWidget::mousePressEvent(QMouseEvent* event) {
    // If the left mouse button is pressed
    if (event->button() == Qt::LeftButton) {
        // Store the global position of the mouse cursor
        oldPos = event->globalPos();
    }
}

void PriceWidget::mouseMoveEvent(QMouseEvent* event) {
    // If the left mouse button is held down while moving the mouse
    if (event->buttons() & Qt::LeftButton) {
        // Calculate the difference between the current mouse position and the previous position
        QPoint delta = event->globalPos() - oldPos;
        // Move the widget by the calculated difference
        move(pos() + delta);
        // Update the stored mouse position to the current position
        oldPos = event->globalPos();
    }
}

void PriceWidget::contextMenuEvent(QContextMenuEvent*) {
    // Toggle between green and purple
    static bool isGreen = true;
    QString newColor = isGreen ? "rgb(255, 0, 255)" : "rgb(0, 255, 0)";
    
    for (auto* label : priceLabels) {
        label->setStyleSheet(QString("color: %1").arg(newColor));
    }
    
    isGreen = !isGreen;
}

std::vector<QString> PriceWidget::loadSymbolsFromConfig() {
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString configPath = homePath + "/.pricetrack.json";
    QFile configFile(configPath);

    if (!configFile.exists()) {
        qDebug() << "Config file not found, creating with default symbols:" << defaultSymbols;
        createDefaultConfig(configPath);
        return defaultSymbols;
    }

    if (!configFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Error opening config file:" << configFile.errorString();
        return defaultSymbols;
    }

    QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
    configFile.close();

    if (!doc.isArray()) {
        qDebug() << "Invalid config format, using defaults";
        return defaultSymbols;
    }

    std::vector<QString> loadedSymbols;
    QJsonArray array = doc.array();
    for (const auto& value : array) {
        if (value.isString()) {
            loadedSymbols.push_back(value.toString());
        }
    }

    return loadedSymbols.empty() ? defaultSymbols : loadedSymbols;
}

void PriceWidget::createDefaultConfig(const QString& path) {
    QJsonArray array;
    for (const auto& symbol : defaultSymbols) {
        array.append(symbol);
    }

    QJsonDocument doc(array);
    QFile configFile(path);
    if (configFile.open(QIODevice::WriteOnly)) {
        configFile.write(doc.toJson());
        configFile.close();
    } else {
        qDebug() << "Failed to create config file:" << configFile.errorString();
    }
}