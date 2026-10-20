#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "ContourMatching.h"
#include "RadioControls.h"

namespace fs = std::filesystem;

enum class Detector { Sobel = 0, Laplace = 1, Canny = 2 };

struct Parameters {
    Detector detector = Detector::Canny;
    // Sobel/Laplace : seul le premier seuil est utilisé.
    // Canny : premier seuil = bas, second seuil = haut.
    int firstThreshold = 50;
    int secondThreshold = 150;
    int kernel = 3;
};

struct Measures {
    long long detected = 0;
    long long reference = 0;
    long long correct = 0;
    long long falsePositive = 0;
    long long falseNegative = 0;
    double performance = 0.0;
    double falsePositiveRate = 0.0;
    double falseNegativeRate = 0.0;
};

std::string detectorName(Detector detector) {
    switch (detector) {
    case Detector::Sobel: return "Sobel";
    case Detector::Laplace: return "Laplace";
    case Detector::Canny: return "Canny";
    }
    return "Unknown";
}

Detector detectorFromNumber(int value) {
    return static_cast<Detector>(std::clamp(value, 0, 2));
}

void validateParameters(const Parameters& parameters) {
    if (parameters.firstThreshold < 0 || parameters.secondThreshold < 0)
        throw std::invalid_argument("Les seuils doivent etre positifs ou nuls.");
    if (parameters.kernel != 1 && parameters.kernel != 3 &&
        parameters.kernel != 5 && parameters.kernel != 7)
        throw std::invalid_argument("Noyau autorise : 1, 3, 5 ou 7.");
    if (parameters.detector == Detector::Canny && parameters.kernel == 1)
        throw std::invalid_argument("Canny accepte seulement les noyaux 3, 5 et 7.");
    if (parameters.detector == Detector::Canny && parameters.firstThreshold > parameters.secondThreshold)
        throw std::invalid_argument("Canny : le seuil bas doit etre inferieur ou egal au seuil haut.");
}

cv::Mat detectEdges(const cv::Mat& image, const Parameters& parameters) {
    validateParameters(parameters);
    cv::Mat edges;
    const int kernel = parameters.kernel;

    if (parameters.detector == Detector::Sobel) {
        // Deux directions de variation, puis une seule force de contour.
        cv::Mat gradientX, gradientY, magnitude;
        cv::Sobel(image, gradientX, CV_32F, 1, 0, kernel);
        cv::Sobel(image, gradientY, CV_32F, 0, 1, kernel);
        cv::magnitude(gradientX, gradientY, magnitude);
        cv::compare(magnitude, parameters.firstThreshold, edges, cv::CMP_GE);
    } else if (parameters.detector == Detector::Laplace) {
        // La valeur absolue garde les variations de contraste des deux signes.
        cv::Mat laplacian, absoluteLaplacian;
        cv::Laplacian(image, laplacian, CV_32F, kernel);
        cv::absdiff(laplacian, cv::Scalar::all(0), absoluteLaplacian);
        cv::compare(absoluteLaplacian, parameters.firstThreshold, edges, cv::CMP_GE);
    } else {
        cv::Canny(image, edges, parameters.firstThreshold, parameters.secondThreshold, kernel);
    }

    return edges;
}

Measures compareEdges(const cv::Mat& detected, const cv::Mat& reference) {
    if (detected.empty() || detected.size() != reference.size() ||
        detected.type() != CV_8UC1 || reference.type() != CV_8UC1)
        throw std::invalid_argument("La comparaison exige deux masques 8 bits de meme taille.");
    cv::Mat detectedBinary, referenceBinary;
    cv::threshold(detected, detectedBinary, 0, 255, cv::THRESH_BINARY);
    // Les références fournies ont leurs contours en noir : on les inverse.
    cv::threshold(reference, referenceBinary, 0, 255, cv::THRESH_BINARY_INV);

    Measures result;
    result.detected = cv::countNonZero(detectedBinary);
    result.reference = cv::countNonZero(referenceBinary);

    result.correct = countContourPairs(detectedBinary, referenceBinary);
    result.falsePositive = result.detected - result.correct;
    result.falseNegative = result.reference - result.correct;

    const long long total = result.correct + result.falsePositive + result.falseNegative;
    if (total != 0) {
        result.performance = static_cast<double>(result.correct) / total;
        result.falsePositiveRate = static_cast<double>(result.falsePositive) / total;
        result.falseNegativeRate = static_cast<double>(result.falseNegative) / total;
    }
    return result;
}

void printMeasuresHeader(std::ostream& output) {
    output << "image,detector,first_parameter,second_parameter,kernel,"
              "contours_detectes,contours_reference,contours_corrects,faux_positifs,"
              "faux_negatifs,performance,TFP,TFN\n";
}

void printMeasures(std::ostream& output, const std::string& imageName,
                   const Parameters& parameters, const Measures& measures) {
    output << imageName << ',' << detectorName(parameters.detector) << ','
           << parameters.firstThreshold << ',' << parameters.secondThreshold << ',' << parameters.kernel << ','
           << measures.detected << ',' << measures.reference << ',' << measures.correct << ','
           << measures.falsePositive << ',' << measures.falseNegative << ','
           << std::fixed << std::setprecision(6) << measures.performance << ','
           << measures.falsePositiveRate << ',' << measures.falseNegativeRate << '\n';
}

bool isInputImage(const fs::path& path) {
    return path.extension() == ".pgm" && path.filename().string().find("_gt_") == std::string::npos;
}

bool runBatch(const fs::path& imageDirectory, const fs::path& referenceDirectory,
              const fs::path& outputDirectory, const Parameters& parameters) {
    validateParameters(parameters);
    if (!fs::is_directory(imageDirectory) || !fs::is_directory(referenceDirectory))
        throw std::runtime_error("Dossier d'images ou de references absent.");
    fs::create_directories(outputDirectory);
    std::ofstream csv(outputDirectory / "mesures.csv");
    if (!csv) throw std::runtime_error("Impossible de creer le CSV dans " + outputDirectory.string());
    printMeasuresHeader(csv);

    // Un ordre fixe facilite la comparaison de deux executions.
    std::vector<fs::path> images;
    for (const auto& entry : fs::directory_iterator(imageDirectory)) {
        if (entry.is_regular_file() && isInputImage(entry.path())) images.push_back(entry.path());
    }
    std::sort(images.begin(), images.end());
    int imageCount = 0, errorCount = 0;
    for (const fs::path& imagePath : images) {
        const std::string stem = imagePath.stem().string();
        const fs::path referencePath = referenceDirectory / (stem + "_gt_binary.pgm");
        if (!fs::exists(referencePath)) {
            std::cerr << "Reference absente pour " << imagePath.filename() << '\n';
            ++errorCount;
            continue;
        }
        const cv::Mat image = cv::imread(imagePath.string(), cv::IMREAD_GRAYSCALE);
        const cv::Mat reference = cv::imread(referencePath.string(), cv::IMREAD_GRAYSCALE);
        if (image.empty() || reference.empty() || image.size() != reference.size()) {
            std::cerr << "Image incompatible : " << imagePath << '\n';
            ++errorCount;
            continue;
        }
        const cv::Mat edges = detectEdges(image, parameters);
        const Measures measures = compareEdges(edges, reference);
        const fs::path edgesPath = outputDirectory / (stem + "_edges.pgm");
        if (!cv::imwrite(edgesPath.string(), edges))
            throw std::runtime_error("Impossible d'ecrire " + edgesPath.string());
        printMeasures(csv, imagePath.filename().string(), parameters, measures);
        if (!csv) throw std::runtime_error("Erreur d'ecriture du CSV.");
        ++imageCount;
    }
    csv.close();
    if (!csv) throw std::runtime_error("Erreur de fermeture du CSV.");
    std::cout << imageCount << " image(s) traitee(s), " << errorCount << " en erreur, avec "
              << detectorName(parameters.detector) << ". Resultats : "
              << (outputDirectory / "mesures.csv") << '\n';
    return imageCount > 0 && errorCount == 0;
}

// L'interface propose les noyaux communs 3/5/7 et trie les seuils Canny.
// Le texte affiche toujours les parametres qui seront effectivement appliques.
Parameters interactiveParameters(int detector, int firstThreshold, int secondThreshold,
                                 int kernelIndex, int scaleIndex = 0) {
    Parameters parameters;
    parameters.detector = detectorFromNumber(detector);
    int scale = 1;
    for (int i = 0; i < scaleIndex; ++i) scale *= 10;
    parameters.firstThreshold = firstThreshold * scale;
    parameters.secondThreshold = secondThreshold * scale;
    parameters.kernel = 3 + 2 * kernelIndex;
    if (parameters.detector == Detector::Canny &&
        parameters.firstThreshold > parameters.secondThreshold) {
        std::swap(parameters.firstThreshold, parameters.secondThreshold);
    }
    validateParameters(parameters);
    return parameters;
}

void runInteractive(const fs::path& imagePath) {
    const cv::Mat image = cv::imread(imagePath.string(), cv::IMREAD_GRAYSCALE);
    if (image.empty()) throw std::runtime_error("Impossible de lire " + imagePath.string());

    const std::string window = "Detection de contours";
    cv::namedWindow(window, cv::WINDOW_NORMAL);
    cv::createTrackbar("seuil 1", window, nullptr, 2000);
    cv::createTrackbar("seuil 2 (Canny)", window, nullptr, 2000);
    cv::setTrackbarPos("seuil 1", window, 50);
    cv::setTrackbarPos("seuil 2 (Canny)", window, 150);
    // HighGUI ouvre sinon une petite fenêtre 320x240, illisible pour la démo.
    cv::resizeWindow(window, std::max(590, image.cols * 2), image.rows + 305);
    RadioControls controls = makeRadioControls(image.rows + 55);
    cv::setMouseCallback(window, onRadioClick, &controls);

    // Toujours afficher une image avant de traiter les événements de la fenêtre.
    while (true) {
        const Parameters parameters = interactiveParameters(
            controls.selected[0],
            cv::getTrackbarPos("seuil 1", window),
            cv::getTrackbarPos("seuil 2 (Canny)", window),
            controls.selected[1], controls.selected[2]);
        const cv::Mat edges = detectEdges(image, parameters);
        cv::Mat display, colorDisplay;
        cv::hconcat(image, edges, display);
        cv::cvtColor(display, colorDisplay, cv::COLOR_GRAY2BGR);
        // Largeur minimale pour que tous les boutons restent visibles.
        const int rightBorder = std::max(0, 590 - colorDisplay.cols);
        cv::copyMakeBorder(colorDisplay, colorDisplay, 55, 140, 0, rightBorder,
                           cv::BORDER_CONSTANT, cv::Scalar(30, 30, 30));
        std::string caption = detectorName(parameters.detector) + " | noyau " +
                              std::to_string(parameters.kernel) + " | seuil " +
                              std::to_string(parameters.firstThreshold);
        caption += parameters.detector == Detector::Canny
            ? "/" + std::to_string(parameters.secondThreshold) + " (bas/haut)"
            : " | seuil 2 inactif";
        cv::putText(colorDisplay, caption, cv::Point(8, 22), cv::FONT_HERSHEY_SIMPLEX,
                    0.45, cv::Scalar(255, 255, 255), 1);
        cv::putText(colorDisplay, "Source a gauche | contours a droite | Echap/q : quitter",
                    cv::Point(8, 44), cv::FONT_HERSHEY_SIMPLEX, 0.4,
                    cv::Scalar(255, 255, 255), 1);
        drawRadioControls(colorDisplay, controls);
        cv::imshow(window, colorDisplay);
        const int key = cv::waitKey(30);
        if (key == 27 || key == 'q' || key == 'Q') break;
        // GTK ne fournit pas WND_PROP_VISIBLE. AUTOSIZE vaut ici 0 tant
        // que notre fenêtre WINDOW_NORMAL existe, et -1 après sa fermeture.
        // Certains backends signalent plutôt la disparition par une exception.
        try {
            if (cv::getWindowProperty(window, cv::WND_PROP_AUTOSIZE) < 0) break;
        } catch (const cv::Exception&) {
            break;
        }
    }
    cv::destroyAllWindows();
}

Detector parseDetector(const std::string& name) {
    if (name == "sobel") return Detector::Sobel;
    if (name == "laplace") return Detector::Laplace;
    if (name == "canny") return Detector::Canny;
    throw std::invalid_argument("Detecteur inconnu : " + name);
}

void printUsage(const char* program) {
    std::cout << "Usage:\n"
              << "  " << program << " --batch [images] [references] [output] [detector] [seuil1] [seuil2] [noyau]\n"
              << "  " << program << " --interactive image.pgm\n\n"
              << "detector: sobel, laplace ou canny (defaut: canny)\n"
              << "seuils >= 0 ; Canny : seuil1 <= seuil2 ; seuil2 inactif pour Sobel/Laplace\n"
              << "noyau: 1/3/5/7 pour Sobel/Laplace, 3/5/7 pour Canny\n";
}

int parseInteger(const std::string& text) {
    size_t used = 0;
    const int value = std::stoi(text, &used);
    if (used != text.size()) throw std::invalid_argument("Entier invalide : " + text);
    return value;
}

int main(int argc, char** argv) {
    try {
        if (argc >= 2 && std::string(argv[1]) == "--interactive") {
            if (argc > 3) throw std::invalid_argument("Trop d arguments pour --interactive.");
            runInteractive(argc >= 3 ? argv[2] : "images/bear.pgm");
            return 0;
        }
        if (argc >= 2 && std::string(argv[1]) == "--batch") {
            if (argc > 9) throw std::invalid_argument("Trop d arguments pour --batch.");
            const fs::path images = argc >= 3 ? argv[2] : "images";
            const fs::path references = argc >= 4 ? argv[3] : images / "gt";
            const fs::path output = argc >= 5 ? argv[4] : "resultats";
            const Detector detector = argc >= 6 ? parseDetector(argv[5]) : Detector::Canny;
            Parameters parameters;
            parameters.detector = detector;
            if (argc >= 7) parameters.firstThreshold = parseInteger(argv[6]);
            if (argc >= 8) parameters.secondThreshold = parseInteger(argv[7]);
            if (argc >= 9) parameters.kernel = parseInteger(argv[8]);
            return runBatch(images, references, output, parameters) ? 0 : 1;
        }
        printUsage(argv[0]);
        return argc == 1 || std::string(argv[1]) == "--help" ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Erreur : " << error.what() << '\n';
        return 1;
    }
}
