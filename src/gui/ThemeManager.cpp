#include "ScanWorker.hpp"
#include <QApplication>
#include <QPalette>
#include <QColor>

namespace gcanner {

ThemeManager::ThemeManager() : m_theme(Dark) {
    m_darkStyle = R"(
        QMainWindow, QWidget { background-color: #1e1e2e; color: #cdd6f4; }
        QMenuBar { background-color: #181825; color: #cdd6f4; border: none; padding: 4px; }
        QMenuBar::item { background: transparent; padding: 6px 12px; border-radius: 4px; }
        QMenuBar::item:selected { background-color: #313244; }
        QMenu { background-color: #1e1e2e; border: 1px solid #313244; border-radius: 6px; padding: 4px; }
        QMenu::item { padding: 8px 24px; border-radius: 4px; }
        QMenu::item:selected { background-color: #313244; }
        QMenu::separator { height: 1px; background: #313244; margin: 4px 8px; }

        QToolBar { background-color: #181825; border: none; spacing: 8px; padding: 8px; }
        QToolBar::separator { background: #313244; width: 1px; margin: 4px; }
        QToolButton { background: transparent; border: none; border-radius: 6px; padding: 8px 12px; color: #cdd6f4; }
        QToolButton:hover { background-color: #313244; }
        QToolButton:pressed { background-color: #45475a; }

        QStatusBar { background-color: #181825; border-top: 1px solid #313244; color: #a6adc8; }
        QProgressBar { border: none; border-radius: 4px; background-color: #313244; text-align: center; color: #cdd6f4; height: 16px; }
        QProgressBar::chunk { background-color: #89b4fa; border-radius: 4px; }

        QTabWidget::pane { border: 1px solid #313244; border-radius: 6px; background: #1e1e2e; }
        QTabBar::tab { background: #181825; color: #a6adc8; padding: 10px 20px; border: 1px solid #313244; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }
        QTabBar::tab:selected { background: #1e1e2e; color: #cdd6f4; border-bottom: 1px solid #1e1e2e; }
        QTabBar::tab:hover { background: #313244; }

        QPushButton { background-color: #313244; color: #cdd6f4; border: none; border-radius: 6px; padding: 10px 20px; font-weight: 500; }
        QPushButton:hover { background-color: #45475a; }
        QPushButton:pressed { background-color: #585b70; }
        QPushButton:disabled { background-color: #1e1e2e; color: #6c6f85; }
        QPushButton#PrimaryButton { background-color: #89b4fa; color: #1e1e2e; }
        QPushButton#PrimaryButton:hover { background-color: #74c7ec; }

        QLineEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; padding: 8px; selection-background-color: #89b4fa; }
        QLineEdit:focus, QTextEdit:focus, QComboBox:focus { border-color: #89b4fa; }
        QComboBox::drop-down { border: none; width: 24px; }
        QComboBox QAbstractItemView { background: #1e1e2e; border: 1px solid #313244; selection-background-color: #313244; }

        QGroupBox { color: #cdd6f4; border: 1px solid #313244; border-radius: 8px; margin-top: 12px; padding-top: 16px; font-weight: bold; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: #89b4fa; }

        QTreeWidget, QListWidget, QTableWidget { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; alternate-background-color: #1e1e2e; show-decoration-selected: 1; }
        QTreeWidget::item, QListWidget::item { padding: 8px; border-radius: 4px; }
        QTreeWidget::item:selected, QListWidget::item:selected { background-color: #313244; color: #cdd6f4; }
        QHeaderView::section { background-color: #181825; color: #a6adc8; border: none; border-right: 1px solid #313244; border-bottom: 1px solid #313244; padding: 8px; font-weight: bold; }

        QScrollBar:vertical { background: #181825; width: 10px; border: none; }
        QScrollBar::handle:vertical { background: #45475a; border-radius: 5px; min-height: 30px; }
        QScrollBar::handle:vertical:hover { background: #585b70; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

        QFrame#FeatureCard { background-color: #181825; border: 1px solid #313244; border-radius: 12px; }
        QFrame#FeatureCard:hover { border-color: #89b4fa; }

        QLabel#TitleLabel { color: #cdd6f4; }
        QLabel#SubtitleLabel { color: #a6adc8; }

        QSplitter::handle { background: #313244; }
        QSplitter::handle:horizontal { width: 2px; }
        QSplitter::handle:vertical { height: 2px; }

        QDialog { background-color: #1e1e2e; }
        QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid #313244; border-radius: 4px; background: #181825; }
        QCheckBox::indicator:checked { background-color: #89b4fa; border-color: #89b4fa; image: url(:/icons/check.svg); }
        QRadioButton::indicator { width: 18px; height: 18px; border: 2px solid #313244; border-radius: 9px; background: #181825; }
        QRadioButton::indicator:checked { background-color: #89b4fa; border-color: #89b4fa; }

        QChartView { background: #181825; }
    )";

    m_lightStyle = R"(
        QMainWindow, QWidget { background-color: #f5f5f5; color: #1e1e2e; }
        QMenuBar { background-color: #ffffff; color: #1e1e2e; border: none; padding: 4px; }
        QMenuBar::item { background: transparent; padding: 6px 12px; border-radius: 4px; }
        QMenuBar::item:selected { background-color: #e8e8e8; }
        QMenu { background-color: #ffffff; border: 1px solid #d0d0d0; border-radius: 6px; padding: 4px; }
        QMenu::item { padding: 8px 24px; border-radius: 4px; }
        QMenu::item:selected { background-color: #e8e8e8; }
        QMenu::separator { height: 1px; background: #d0d0d0; margin: 4px 8px; }

        QToolBar { background-color: #ffffff; border: none; spacing: 8px; padding: 8px; border-bottom: 1px solid #d0d0d0; }
        QToolBar::separator { background: #d0d0d0; width: 1px; margin: 4px; }
        QToolButton { background: transparent; border: none; border-radius: 6px; padding: 8px 12px; color: #1e1e2e; }
        QToolButton:hover { background-color: #e8e8e8; }
        QToolButton:pressed { background-color: #d0d0d0; }

        QStatusBar { background-color: #ffffff; border-top: 1px solid #d0d0d0; color: #666; }
        QProgressBar { border: none; border-radius: 4px; background-color: #e8e8e8; text-align: center; color: #1e1e2e; height: 16px; }
        QProgressBar::chunk { background-color: #3b82f6; border-radius: 4px; }

        QTabWidget::pane { border: 1px solid #d0d0d0; border-radius: 6px; background: #f5f5f5; }
        QTabBar::tab { background: #ffffff; color: #666; padding: 10px 20px; border: 1px solid #d0d0d0; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }
        QTabBar::tab:selected { background: #f5f5f5; color: #1e1e2e; border-bottom: 1px solid #f5f5f5; }
        QTabBar::tab:hover { background: #f0f0f0; }

        QPushButton { background-color: #e8e8e8; color: #1e1e2e; border: none; border-radius: 6px; padding: 10px 20px; font-weight: 500; }
        QPushButton:hover { background-color: #d0d0d0; }
        QPushButton:pressed { background-color: #b8b8b8; }
        QPushButton:disabled { background-color: #f5f5f5; color: #999; }
        QPushButton#PrimaryButton { background-color: #3b82f6; color: #ffffff; }
        QPushButton#PrimaryButton:hover { background-color: #2563eb; }

        QLineEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox { background-color: #ffffff; color: #1e1e2e; border: 1px solid #d0d0d0; border-radius: 6px; padding: 8px; selection-background-color: #3b82f6; }
        QLineEdit:focus, QTextEdit:focus, QComboBox:focus { border-color: #3b82f6; }
        QComboBox::drop-down { border: none; width: 24px; }
        QComboBox QAbstractItemView { background: #ffffff; border: 1px solid #d0d0d0; selection-background-color: #e8e8e8; }

        QGroupBox { color: #1e1e2e; border: 1px solid #d0d0d0; border-radius: 8px; margin-top: 12px; padding-top: 16px; font-weight: bold; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: #3b82f6; }

        QTreeWidget, QListWidget, QTableWidget { background-color: #ffffff; color: #1e1e2e; border: 1px solid #d0d0d0; border-radius: 6px; alternate-background-color: #f5f5f5; show-decoration-selected: 1; }
        QTreeWidget::item, QListWidget::item { padding: 8px; border-radius: 4px; }
        QTreeWidget::item:selected, QListWidget::item:selected { background-color: #e8e8e8; color: #1e1e2e; }
        QHeaderView::section { background-color: #ffffff; color: #666; border: none; border-right: 1px solid #d0d0d0; border-bottom: 1px solid #d0d0d0; padding: 8px; font-weight: bold; }

        QScrollBar:vertical { background: #f5f5f5; width: 10px; border: none; }
        QScrollBar::handle:vertical { background: #c0c0c0; border-radius: 5px; min-height: 30px; }
        QScrollBar::handle:vertical:hover { background: #a0a0a0; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

        QFrame#FeatureCard { background-color: #ffffff; border: 1px solid #d0d0d0; border-radius: 12px; }
        QFrame#FeatureCard:hover { border-color: #3b82f6; }

        QLabel#TitleLabel { color: #1e1e2e; }
        QLabel#SubtitleLabel { color: #666; }

        QSplitter::handle { background: #d0d0d0; }
        QSplitter::handle:horizontal { width: 2px; }
        QSplitter::handle:vertical { height: 2px; }

        QDialog { background-color: #f5f5f5; }
        QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid #d0d0d0; border-radius: 4px; background: #ffffff; }
        QCheckBox::indicator:checked { background-color: #3b82f6; border-color: #3b82f6; image: url(:/icons/check.svg); }
        QRadioButton::indicator { width: 18px; height: 18px; border: 2px solid #d0d0d0; border-radius: 9px; background: #ffffff; }
        QRadioButton::indicator:checked { background-color: #3b82f6; border-color: #3b82f6; }

        QChartView { background: #ffffff; }
    )";

    applyTheme();
}

ThemeManager& ThemeManager::instance() {
    static ThemeManager instance;
    return instance;
}

void ThemeManager::setTheme(Theme theme) {
    if (m_theme == theme) return;
    m_theme = theme;
    applyTheme();
    emit themeChanged(theme);
}

ThemeManager::Theme ThemeManager::theme() const {
    return m_theme;
}

QString ThemeManager::styleSheet() const {
    return (m_theme == Dark) ? m_darkStyle : m_lightStyle;
}

void ThemeManager::applyTheme() {
    qApp->setStyleSheet(styleSheet());

    // Update palette for native dialogs
    QPalette palette;
    if (m_theme == Dark) {
        palette.setColor(QPalette::Window, QColor("#1e1e2e"));
        palette.setColor(QPalette::WindowText, QColor("#cdd6f4"));
        palette.setColor(QPalette::Base, QColor("#181825"));
        palette.setColor(QPalette::AlternateBase, QColor("#1e1e2e"));
        palette.setColor(QPalette::ToolTipBase, QColor("#1e1e2e"));
        palette.setColor(QPalette::ToolTipText, QColor("#cdd6f4"));
        palette.setColor(QPalette::Text, QColor("#cdd6f4"));
        palette.setColor(QPalette::Button, QColor("#313244"));
        palette.setColor(QPalette::ButtonText, QColor("#cdd6f4"));
        palette.setColor(QPalette::BrightText, QColor("#f38ba8"));
        palette.setColor(QPalette::Link, QColor("#89b4fa"));
        palette.setColor(QPalette::Highlight, QColor("#89b4fa"));
        palette.setColor(QPalette::HighlightedText, QColor("#1e1e2e"));
    } else {
        palette.setColor(QPalette::Window, QColor("#f5f5f5"));
        palette.setColor(QPalette::WindowText, QColor("#1e1e2e"));
        palette.setColor(QPalette::Base, QColor("#ffffff"));
        palette.setColor(QPalette::AlternateBase, QColor("#f5f5f5"));
        palette.setColor(QPalette::ToolTipBase, QColor("#ffffff"));
        palette.setColor(QPalette::ToolTipText, QColor("#1e1e2e"));
        palette.setColor(QPalette::Text, QColor("#1e1e2e"));
        palette.setColor(QPalette::Button, QColor("#e8e8e8"));
        palette.setColor(QPalette::ButtonText, QColor("#1e1e2e"));
        palette.setColor(QPalette::BrightText, QColor("#dc2626"));
        palette.setColor(QPalette::Link, QColor("#3b82f6"));
        palette.setColor(QPalette::Highlight, QColor("#3b82f6"));
        palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    }
    qApp->setPalette(palette);
}

} // namespace gcanner