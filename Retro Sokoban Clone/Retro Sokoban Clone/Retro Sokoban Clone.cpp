//Uwaga! Co najmniej C++17!!!
//Project-> ... Properties->Configuration Properties->General->C++ Language Standard = ISO C++ 17 Standard (/std:c++17)

#include "SFML/Graphics.hpp"
#include "SFML/System/Clock.hpp" 
#include <fstream>

enum class Field { VOID, FLOOR, WALL, BOX, PARK, PLAYER };

class Sokoban : public sf::Drawable
{
public:
	Sokoban()
	{
		loadTextures();
	}
	void LoadMapFromFile(std::string fileName);
	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
	void SetDrawParameters(sf::Vector2u draw_area_size);
	void Move_Player_Left();
	void Move_Player_Right();
	void Move_Player_Up();
	void Move_Player_Down();
	bool Is_Victory() const;

	void UndoMove();
	void loadTextures();
	void WinGame();

private:
	std::vector<std::vector<Field>> map;
	sf::Vector2f shift, tile_size;
	sf::Vector2i player_position;
	std::vector<sf::Vector2i> park_positions;

	std::pair<std::vector<std::vector<Field>>, sf::Vector2i> lastMove; // Ostatni ruch
	bool lastMoveSaved = false; // Czy ostatni ruch został zapisany
	sf::Texture textureWall;
	sf::Texture textureFloor;
	sf::Texture textureBox;
	sf::Texture texturePark;
	sf::Texture texturePlayer;

	void move_player(int dx, int dy);
};

void Sokoban::LoadMapFromFile(std::string fileName)
{
	std::string str;
	std::vector<std::string> vos;

	std::ifstream in(fileName.c_str());
	while (std::getline(in, str)) { vos.push_back(str); }
	in.close();

	map.clear();
	map.resize(vos.size(), std::vector<Field>(vos[0].size()));
	for (auto [row, row_end, y] = std::tuple{ vos.cbegin(), vos.cend(), 0 }; row != row_end; ++row, ++y)
		for (auto [element, end, x] = std::tuple{ row->begin(), row->end(), 0 }; element != end; ++element, ++x)
			switch (*element)
			{
			case 'X': map[y][x] = Field::WALL; break;
			case '*': map[y][x] = Field::VOID; break;
			case ' ': map[y][x] = Field::FLOOR; break;
			case 'B': map[y][x] = Field::BOX; break;
			case 'P': map[y][x] = Field::PARK; park_positions.push_back(sf::Vector2i(x, y));  break;
			case 'S': map[y][x] = Field::PLAYER; player_position = sf::Vector2i(x, y);  break;
			}
}

void Sokoban::draw(sf::RenderTarget& target, sf::RenderStates states) const {

	int boardWidth = map[0].size() * tile_size.x; // Całkowita szerokość planszy
	int boardHeight = map.size() * tile_size.y; // Całkowita wysokość planszy

	// Oblicz przesunięcie, aby wyśrodkować planszę
	sf::Vector2u windowSize = target.getSize();
	float offsetX = (windowSize.x - boardWidth) / 2.0f;
	float offsetY = (windowSize.y - boardHeight) / 2.0f;

	for (int y = 0; y < map.size(); ++y) {
		for (int x = 0; x < map[y].size(); ++x) {
			sf::Sprite sprite;
			switch (map[y][x]) {
			case Field::WALL:
				sprite.setTexture(textureWall);
				break;
			case Field::FLOOR:
				sprite.setTexture(textureFloor);
				break;
			case Field::BOX:
				sprite.setTexture(textureBox);
				break;
			case Field::PARK:
				sprite.setTexture(texturePark);
				break;
			case Field::PLAYER:
				sprite.setTexture(texturePlayer);
				break;
			default:
				continue;
			}

			// Skalowanie sprite'a do rozmiaru kafelka
			float scaleX = tile_size.x / sprite.getTexture()->getSize().x;
			float scaleY = tile_size.y / sprite.getTexture()->getSize().y;
			sprite.setScale(scaleX, scaleY);

			sprite.setPosition(x * tile_size.x, y * tile_size.y);
			target.draw(sprite, states);
		}
	}
}

void Sokoban::SetDrawParameters(sf::Vector2u draw_area_size)
{
	this->tile_size = sf::Vector2f(
		std::min(std::floor((float)draw_area_size.x / (float)map[0].size()), std::floor((float)draw_area_size.y / (float)map.size())),
		std::min(std::floor((float)draw_area_size.x / (float)map[0].size()), std::floor((float)draw_area_size.y / (float)map.size()))
	);
	this->shift = sf::Vector2f(
		((float)draw_area_size.x - this->tile_size.x * map[0].size()) / 2.0f,
		((float)draw_area_size.y - this->tile_size.y * map.size()) / 2.0f
	);
}

void Sokoban::Move_Player_Left()
{
	move_player(-1, 0);
}

void Sokoban::Move_Player_Right()
{
	move_player(1, 0);
}

void Sokoban::Move_Player_Up()
{
	move_player(0, -1);
}

void Sokoban::Move_Player_Down()
{
	move_player(0, 1);
}

void Sokoban::move_player(int dx, int dy)
{
	bool allow_move = false; // Początkowo zakładamy, że ruch nie jest dozwolony.
	sf::Vector2i new_pp(player_position.x + dx, player_position.y + dy); // Nowa pozycja gracza.

	// Sprawdzamy, czy nowa pozycja jest w granicach mapy.
	if (new_pp.x < 0 || new_pp.y < 0 || new_pp.y >= map.size() || new_pp.x >= map[0].size())
		return; // Jeśli nowa pozycja jest poza mapą, kończymy funkcję.

	Field fts = map[new_pp.y][new_pp.x]; // Element na miejscu, na które gracz zamierza przejść.

	// Gracz może się poruszyć, jeśli pole na którym ma stanąć to podłoga lub miejsce na skrzynki.
	if (fts == Field::FLOOR || fts == Field::PARK)
		allow_move = true;

	// Jeśli gracz chce wejść na pole z skrzynką.
	if (fts == Field::BOX)
	{
		sf::Vector2i box_new_pos(new_pp.x + dx, new_pp.y + dy); // Nowa pozycja skrzyni.
		// Sprawdzamy, czy nowa pozycja skrzyni jest w granicach mapy.
		if (box_new_pos.x >= 0 && box_new_pos.y >= 0 && box_new_pos.y < map.size() && box_new_pos.x < map[0].size())
		{
			Field ftsa = map[box_new_pos.y][box_new_pos.x]; // Element za skrzynią.

			// Możemy przesunąć skrzynkę tylko jeśli kolejne pole jest podłogą lub miejscem na skrzynkę.
			if (ftsa == Field::FLOOR || ftsa == Field::PARK)
			{
				allow_move = true;
				// Przesuwamy skrzynkę.
				map[box_new_pos.y][box_new_pos.x] = Field::BOX;
				// Pole, z którego przesunęliśmy skrzynkę, staje się podłogą.
				map[new_pp.y][new_pp.x] = Field::FLOOR;
			}
		}
	}

	if (allow_move)
	{
		if (!lastMoveSaved) {
			lastMove = { map, player_position };
			lastMoveSaved = true;
		}
		// Przesuwamy gracza.
		map[player_position.y][player_position.x] = Field::FLOOR;
		player_position = new_pp;
		map[player_position.y][player_position.x] = Field::PLAYER;
	}

	// W przypadku, gdy miejsce na skrzynkę zostało nadpisane przez gracza, przywracamy je.
	for (auto park_position : park_positions)
		if (map[park_position.y][park_position.x] == Field::FLOOR)
			map[park_position.y][park_position.x] = Field::PARK;
}

bool Sokoban::Is_Victory() const
{
	//Tym razem dla odmiany optymistycznie zakładamy, że gracz wygrał.
	//No ale jeśli na którymkolwiek miejscu na skrzynki nie ma skrzynki to chyba założenie było zbyt optymistyczne... : -/
	for (auto park_position : park_positions) if (map[park_position.y][park_position.x] != Field::BOX) return false;
	return true;
}

void Sokoban::UndoMove() {
	if (!lastMoveSaved) return; // Jeśli nie ma zapisanego ruchu, nie cofamy

	// Przywracamy stan mapy i pozycję gracza
	map = lastMove.first;
	player_position = lastMove.second;
	lastMoveSaved = false; // Resetujemy flagę
}

void Sokoban::loadTextures() {
	textureWall.loadFromFile("wall.png");
	textureFloor.loadFromFile("floor.png");
	textureBox.loadFromFile("box.png");
	texturePark.loadFromFile("park.png");
	texturePlayer.loadFromFile("player.png");
}

void Sokoban::WinGame() {
	for (auto& row : map) {
		for (auto& field : row) {
			if (field == Field::BOX) {
				field = Field::FLOOR; // Zmienia boxy na podłogę
			}
		}
	}
	// Ustaw wszystkie park_positions na BOX, symulując zwycięstwo
	for (auto& pos : park_positions) {
		map[pos.y][pos.x] = Field::BOX;
	}
}


int main()
{

	sf::Font font;
	if (!font.loadFromFile("BebasNeue-Regular.ttf"))
	{
		// obsługa błędu, np. wyjście z programu
		return -1;
	}
	sf::Text victoryText("Victory!", font);
	victoryText.setCharacterSize(48); // ustaw rozmiar czcionki
	victoryText.setFillColor(sf::Color::Green); // ustaw kolor tekstu

	sf::Text exitPrompt("Are you giving up already?", font);
	exitPrompt.setCharacterSize(48); // Rozmiar czcionki
	exitPrompt.setFillColor(sf::Color::Green); // Kolor tekstu

	sf::RenderWindow window(sf::VideoMode(800, 600), "GFK Lab 01", sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
	Sokoban sokoban;

	sf::Vector2u windowSize = window.getSize();
	sf::FloatRect exitPromptBounds = exitPrompt.getLocalBounds();
	exitPrompt.setOrigin(exitPromptBounds.width / 2, exitPromptBounds.height / 2);
	exitPrompt.setPosition(windowSize.x / 2.0f, windowSize.y / 2.0f);

	sokoban.LoadMapFromFile("plansza.txt");
	sokoban.SetDrawParameters(window.getSize());

	sf::Clock exitClock;
	bool askForExit = false;
	bool displayExitPrompt = false;

	while (window.isOpen())
	{
		sf::Event event;

		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::Resized)
			{
				// Zaktualizuj widok, aby pasował do nowego rozmiaru okna
				sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
				window.setView(sf::View(visibleArea));

				// Zaktualizuj pozycję tekstów, aby były wyśrodkowane
				sf::Vector2u windowSize = window.getSize();
				victoryText.setPosition(windowSize.x / 2.0f, windowSize.y / 2.0f);
				sf::FloatRect exitPromptBounds = exitPrompt.getLocalBounds();
				exitPrompt.setOrigin(exitPromptBounds.width / 2, exitPromptBounds.height / 2);
				exitPrompt.setPosition(event.size.width / 2.0f, event.size.height / 2.0f);

				sokoban.SetDrawParameters(window.getSize());
			}
			//Oczywiście tu powinny zostać jakoś obsłużone inne zdarzenia.Na przykład jak gracz naciśnie klawisz w lewo powinno pojawić się wywołanie metody :
			//sokoban.Move_Player_Left();
			//W dowolnym momencie mogą Państwo sprawdzić czy gracz wygrał:
			//sokoban.Is_Victory();

			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Escape)
				{
					if (sokoban.Is_Victory())
					{
						// Jeśli gracz wygrał, zamknij grę bez wyświetlania monitu
						window.close();
					}
					else
					{
						if (askForExit && exitClock.getElapsedTime().asSeconds() <= 4.0)
						{
							window.close();
						}
						else
						{
							askForExit = true;
							displayExitPrompt = true; // Włącz wyświetlanie pytania
							exitClock.restart();
						}
					}
				}
			}

			if (askForExit && exitClock.getElapsedTime().asSeconds() > 4.0)
			{
				askForExit = false;
				displayExitPrompt = false; // Wyłącz wyświetlanie pytania
			}

			// Obsługa klawiatury poza pętlą pollEvent
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
			{
				sokoban.Move_Player_Left();
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
			{
				sokoban.Move_Player_Right();
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
			{
				sokoban.Move_Player_Up();
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
			{
				sokoban.Move_Player_Down();
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z) && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
				sokoban.UndoMove();
			}
			else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::V)
			{
				sokoban.WinGame(); // Ustaw stan gry na zwycięstwo
			}

		}

		window.clear(); // Czyści okno przed ponownym narysowaniem

		window.draw(sokoban);

		if (sokoban.Is_Victory())
		{
			// Pobierz rozmiary okna
			sf::Vector2u windowSize = window.getSize();

			// Pobierz rozmiary tekstu
			sf::FloatRect textRect = victoryText.getLocalBounds();

			// Ustaw nową pozycję tekstu, aby był wyśrodkowany w oknie
			victoryText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
			victoryText.setPosition(sf::Vector2f(windowSize.x / 2.0f, windowSize.y / 2.0f));

			window.draw(victoryText); // Rysuje tekst zwycięstwa na środku ekranu
		}

		if (displayExitPrompt)
		{
			window.draw(exitPrompt); // Rysuj pytanie, jeśli ma być wyświetlane
		}

		window.display();


	}

	return 0;
}