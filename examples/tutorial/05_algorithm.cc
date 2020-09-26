#include <ezy/algorithm.h>
#include <vector>
#include <string>

std::vector<std::string> get_words_longer_than(const std::vector<std::string>& words, size_t treshold)
{
  const auto too_long = [treshold](const auto& word) { return word.size() > treshold; };
  return ezy::collect<std::vector<std::string>>(
      ezy::filter(words, too_long)
    );
}

#include <iostream>

int main()
{
  const std::vector<std::string> words{"a", "bb", "looong", "looooong", "with, comma, and, space"};
  const auto long_words = get_words_longer_than(words, 3);
  std::cout << ezy::join(long_words, ", ") << "\n";

  auto quote = [](const std::string &s) { return std::string() + "\"" + s + "\""; };
  std::cout << ezy::join(ezy::transform(long_words, quote), ", ") << "\n";

  //
  const auto print_with_number = [](const auto& element)
  {
    const auto& [i, word] = element;
    std::cout << i << ". " << word << "\n";
  };

  ezy::for_each(ezy::enumerate(long_words), print_with_number);

  ezy::for_each(ezy::zip(ezy::iterate(1), long_words), print_with_number);
  return 4;
}
