#pragma once

#include <array>
#include <string>
#include <vector>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

// Contrôles dessinés dans l'image : fonctionnent aussi avec le backend GTK,
// sans dépendre des boutons réservés au backend Qt d'OpenCV.
struct RadioButton {
    cv::Rect area;
    std::string label;
    int group;
    int value;
};

struct RadioControls {
    // Un seul choix par groupe : détecteur, indice du noyau, indice de l'échelle.
    std::array<int, 3> selected = {2, 0, 0};
    std::vector<RadioButton> buttons;
    int top;
};

inline RadioControls makeRadioControls(int top) {
    RadioControls controls;
    controls.top = top;
    const std::array<std::vector<std::string>, 3> labels = {{
        {"Sobel", "Laplace", "Canny"}, {"3", "5", "7"}, {"1", "10", "100", "1000"}
    }};
    for (int group = 0; group < 3; ++group) {
        for (size_t choice = 0; choice < labels[group].size(); ++choice) {
            controls.buttons.push_back({
                cv::Rect(140 + static_cast<int>(choice) * 110, top + 12 + group * 40, 100, 30),
                labels[group][choice], group, static_cast<int>(choice)
            });
        }
    }
    return controls;
}

inline void onRadioClick(int event, int x, int y, int, void* data) {
    if (event != cv::EVENT_LBUTTONDOWN) return;
    auto& controls = *static_cast<RadioControls*>(data);
    for (const RadioButton& button : controls.buttons) {
        if (button.area.contains(cv::Point(x, y))) {
            controls.selected[button.group] = button.value;
            return;
        }
    }
}

inline void drawRadioControls(cv::Mat& display, const RadioControls& controls) {
    const std::array<std::string, 3> titles = {"Detecteur", "Noyau", "Echelle"};
    for (int group = 0; group < 3; ++group) {
        cv::putText(display, titles[group], cv::Point(12, controls.top + 33 + group * 40),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(230, 230, 230), 1, cv::LINE_AA);
    }
    for (const RadioButton& button : controls.buttons) {
        const bool selected = controls.selected[button.group] == button.value;
        const cv::Scalar color = selected ? cv::Scalar(80, 220, 130) : cv::Scalar(190, 190, 190);
        const cv::Point center(button.area.x + 15, button.area.y + 15);
        cv::circle(display, center, 8, color, 1, cv::LINE_AA);
        if (selected) cv::circle(display, center, 4, color, cv::FILLED, cv::LINE_AA);
        cv::putText(display, button.label, cv::Point(button.area.x + 30, button.area.y + 21),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1, cv::LINE_AA);
    }
}
