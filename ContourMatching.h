#pragma once

#include <vector>
#include <opencv2/core.hpp>

// Une paire relie un contour détecté à une référence distante d'au plus
// un pixel. Chaque pixel n'appartient qu'à une paire.
inline long long countContourPairs(const cv::Mat& detected, const cv::Mat& reference) {
    const int width = detected.cols;
    const int size = static_cast<int>(detected.total());
    // Un indice représente un pixel : indice = y * largeur + x.
    // referenceOf[d] donne la référence de d ; ownerOf[r] donne sa détection.
    // -1 signifie que le pixel est libre.
    std::vector<int> referenceOf(size, -1), ownerOf(size, -1);
    std::vector<int> visited(size, 0), parent(size, -1), queue;
    std::vector<unsigned char> saturated(size, 0);
    long long count = 0;
    int search = 0;

    // Les correspondances exactes constituent un bon point de départ.
    // Elles pourront être déplacées si cela permet de former davantage de paires.
    for (int p = 0; p < size; ++p) {
        if (detected.at<uchar>(p / width, p % width) &&
            reference.at<uchar>(p / width, p % width)) {
            referenceOf[p] = p;
            ownerOf[p] = p;
            ++count;
        }
    }

    for (int root = 0; root < size; ++root) {
        if (!detected.at<uchar>(root / width, root % width) || referenceOf[root] != -1)
            continue;
        ++search;
        queue.clear();
        queue.push_back(root);
        visited[root] = search;
        int freeReference = -1;

        // Recherche en largeur : une référence occupée conduit à son propriétaire.
        // On cherche ainsi une chaîne de déplacements finissant sur une place libre.
        for (size_t head = 0; head < queue.size() && freeReference == -1; ++head) {
            const int p = queue[head];
            for (int dy = -1; dy <= 1 && freeReference == -1; ++dy) {
                for (int dx = -1; dx <= 1 && freeReference == -1; ++dx) {
                    const int y = p / width + dy, x = p % width + dx;
                    if (y < 0 || y >= detected.rows || x < 0 || x >= width ||
                        !reference.at<uchar>(y, x)) continue;
                    const int r = y * width + x;
                    const int owner = ownerOf[r];
                    if (owner == -1) {
                        parent[r] = p;
                        freeReference = r;
                    } else if (!saturated[owner] && visited[owner] != search) {
                        parent[r] = p;
                        visited[owner] = search;
                        queue.push_back(owner);
                    }
                }
            }
        }

        if (freeReference == -1) {
            // Toute cette zone accessible est pleine et n'a aucune sortie libre.
            // Les recherches suivantes peuvent l'éviter : les déplacements ne
            // créent pas de place libre et une nouvelle paire en consomme une.
            for (int p : queue) saturated[p] = 1;
            continue;
        }
        // Remonter la chaîne et déplacer les paires pour libérer une place.
        while (freeReference != -1) {
            const int p = parent[freeReference];
            const int previousReference = referenceOf[p];
            referenceOf[p] = freeReference;
            ownerOf[freeReference] = p;
            freeReference = previousReference;
        }
        ++count;
    }
    return count;
}
