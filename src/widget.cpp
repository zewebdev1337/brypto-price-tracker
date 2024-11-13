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
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);

    fadeTimer.setSingleShot(false);
    fadeTimer.setInterval(16);
    connect(&fadeTimer, &QTimer::timeout, this, [this]() {
        float step = 0.05;
        if (std::abs(currentOpacity - targetOpacity) < step) {
            currentOpacity = targetOpacity;
            fadeTimer.stop();
        } else {
            currentOpacity += (targetOpacity > currentOpacity) ? step : -step;
        }
        opacityEffect->setOpacity(currentOpacity);
        update();
    });
    
    // Connect the finished signal of the network manager to the handleNetworkReply slot
    // This ensures that the handleNetworkReply function is called when a network request is finished
    connect(networkManager.get(), &QNetworkAccessManager::finished,
            this, &PriceWidget::handleNetworkReply);
    
    initUI();
}

void PriceWidget::enterEvent(QEvent *event) {
    updateOpacity(HOVER_OPACITY);
    QWidget::enterEvent(event);
}

void PriceWidget::leaveEvent(QEvent *event) {
    updateOpacity(INACTIVE_OPACITY);
    QWidget::leaveEvent(event);
}

void PriceWidget::focusInEvent(QFocusEvent *event) {
    updateOpacity(ACTIVE_OPACITY);
    QWidget::focusInEvent(event);
}

void PriceWidget::focusOutEvent(QFocusEvent *event) {
    updateOpacity(INACTIVE_OPACITY);
    QWidget::focusOutEvent(event);
}

void PriceWidget::updateOpacity(float target) {
    targetOpacity = target;
    if (!fadeTimer.isActive()) {
        fadeTimer.start();
    }
}

void PriceWidget::initUI() {
    setAttribute(Qt::WA_TranslucentBackground);
    contentWidget = new QWidget(this);
    opacityEffect = new QGraphicsOpacityEffect(this);
    opacityEffect->setOpacity(1.0);
    contentWidget->setGraphicsEffect(opacityEffect);

    layout = new QVBoxLayout(contentWidget);
    layout->setSpacing(2);
    layout->setContentsMargins(10, 10, 10, 10);

    for (const auto& symbol : symbols) {
        auto* label = new QLabel("Loading...", this);
        label->setStyleSheet("color: rgb(255, 255, 255)");
        label->setFont(QFont("Monospace", 12));
        layout->addWidget(label);
        priceLabels[symbol] = label;
    }

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(contentWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

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

void PriceWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QColor backgroundColor(16, 16, 16, static_cast<int>((currentOpacity * 0.5) * 255));
    painter.setBrush(backgroundColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 10, 10);
    opacityEffect->setOpacity(currentOpacity);
    QWidget::paintEvent(event);
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
    static int colorIndex = 0;
    const QStringList colors = {"rgb(255, 255, 255)", "rgb(0, 255, 0)", "rgb(255, 0, 255)"};
    colorIndex = (colorIndex + 1) % colors.size();
    QString newColor = colors[colorIndex];
    for (auto* label : priceLabels) {
        label->setStyleSheet(QString("color: %1").arg(newColor));
    }
}

std::vector<QString> PriceWidget::loadSymbolsFromConfig() {
    QSettings settings("zewebdev1337", "brypto-price-tracker");
    QStringList symbolList = settings.value("symbols").toStringList();
    std::vector<QString> loadedSymbols;

    if (symbolList.isEmpty()) {
        qDebug() << "No symbols found in settings, using defaults:" << defaultSymbols;
        saveSymbolsToConfig(defaultSymbols); // Save defaults if none are found
        return defaultSymbols;
    }

    for (const QString& symbol : symbolList) {
        loadedSymbols.push_back(symbol);
        }
    return loadedSymbols;
}

void PriceWidget::saveSymbolsToConfig(const std::vector<QString>& symbolsToSave) {
    QSettings settings("zewebdev1337", "brypto-price-tracker");
    QStringList symbolList;
    for (const QString& symbol : symbolsToSave) {
        symbolList.append(symbol);
    }
    settings.setValue("symbols", symbolList);
    }
