#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

#define Block_Size 4096

int main() {
    vector<int> random_numbers;
    vector<int> random_number_stride;

    ifstream input_file("random_numbers.txt");
    if (!input_file) {
        cerr << "エラー: random_numbers.txt が見つかりません。" << endl;
        return 1;
    }

    int n;
    while (input_file >> n) {
        random_numbers.push_back(n);
    }
    input_file.close();

    if (random_numbers.size() < 2) {
        cerr << "エラー: 計算に必要なデータ（最低2個）が足りません。" << endl;
        return 1;
    }

    for (size_t i = 0; i < random_numbers.size() - 1; i++) {
        random_number_stride.push_back(Block_Size + random_numbers[i+1] - random_numbers[i]);
    }

    ofstream output_file("random_numbers_stride.txt");
    if (!output_file) {
        cerr << "エラー: 出力ファイルを作成できませんでした。" << endl;
        return 1;
    }

    for (size_t i = 0; i < random_number_stride.size(); i++) {
        output_file << random_number_stride[i] << (i == random_number_stride.size() - 1 ? "" : " ");
    }
    output_file << endl;

    output_file.close();
    cout << "計算結果を random_numbers_stride.txt に保存しました。" << endl;

    return 0;
}