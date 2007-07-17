#ifndef _NODRZE_H
#define _NODRZE_H

//don't look here, it's a horrible, partially working implementation of RB trees

//ignore comment above, it is simply TowDragon's envy. Everything (without removing) is working fine

#include <iostream>
#include <string>

#define LOGUJ ;
#define CLOG ;

#ifndef LOGUJ
#define LOGUJ(a) (std::cout<<a)
#define CLOG(a) (std::cout<<a)
#endif
const bool CZERWONY=true, CZARNY=false;
class TTAICore;
extern TTAICore * C;

template <typename T>  class wezel
{
public:
	bool kolor:1;
	T * zawart;
	wezel * ojciec, *lewy, *prawy;
	wezel(bool kol):kolor(kol),ojciec(NULL),lewy(NULL),prawy(NULL){zawart = new T;};
	wezel(wezel * NIL);
	~wezel(){delete zawart;}
};
template <typename T>  std::ostream & piszAdresy(std::ostream & strum, wezel<T> & w)
{
	strum << "Informacje o wezle: "<<&w;
	strum <<"\n\tOjciec: "<<(w.ojciec);
	strum<<"\n\tLewy syn: "<<(w.lewy);
	strum<<"\n\tPrawy syn: "<<(w.prawy);
	strum<<"\n\tKolor: "<<((w.kolor)?(std::string("Czerwony")):(std::string("Czarny")))<<std::endl<<std::endl;
	return strum;
}
template <typename T>  std::ostream & operator<<(std::ostream & strum, wezel<T> & w)
{
	strum << "Informacje o wezle: "<<&w<<" - "<<*w.zawart;
	strum <<"\n\tOjciec: "<<(w.ojciec)<<" - "<<*w.ojciec->zawart;
	strum<<"\n\tLewy syn: "<<(w.lewy)<<" - "<<*w.lewy->zawart;
	strum<<"\n\tPrawy syn: "<<(w.prawy)<<" - "<<*w.prawy->zawart;
	strum<<"\n\tKolor: "<<((w.kolor)?(std::string("Czerwony")):(std::string("Czarny")))<<std::endl<<std::endl;
	return strum;
}
template <typename T> wezel<T>::wezel(wezel * NIL)
{
	ojciec=NIL; lewy=NIL; prawy=NIL; kolor=CZERWONY; zawart = NULL;
}
template <typename T>  class nodrze
{
private:
	wezel<T> * NIL, *ostatnio;
	int ile, ktory;
	void zepsuj();
	void dodajBSTC (wezel<T> * nowy);
	void dodajBST (T co);
	void dodajRBT (wezel<T> * nowy);
	wezel<T> * usunRBT (wezel<T> * nowy);
	void naprawWstaw (wezel<T> * nowy);
	void naprawUsun (wezel<T> * x);
	wezel<T> * minimum(wezel<T> * w);
	wezel<T> * maksimum(wezel<T> * w);
	wezel<T> * nastepnik(wezel<T> * w);
	wezel<T> * poprzednik(wezel<T> * w);
	wezel<T> * szukajRek(wezel<T> * w, T co);
	wezel<T> * szukajIter(wezel<T> * w, T co);
	void in(std::ostream & strum, wezel<T> * wsk);
	void inIt(std::ostream & strum, wezel<T> * wsk);
	void pre(std::ostream & strum, wezel<T> * wsk);
	void post(std::ostream & strum, wezel<T> * wsk);
	void rotacjaLewa (wezel<T> * x);
	void rotacjaPrawa (wezel<T> * y);
	bool czyBST (wezel<T> * w);
	bool sprawdzW(wezel<T> * w);
	void destrukcja(wezel<T> * w);
	void wypisuj(wezel<T> * w, std::ostream & strum);
	void wypisujPre(wezel<T> * w, std::ostream & strum);
public:
	wezel<T> * korzen; //root
	nodrze():ile(0) //najzwyczajniejszy w swiecie kosntruktor // c-tor
	{
		NIL=new wezel<T>(CZARNY);
		korzen=NIL;
		ostatnio=NIL;
		ktory=0;
	};
	T * begin () {return minimumimum();}; //first element (=minimum)
	T * end () {return NIL;}; // 
	void clear(); // czysci az do korzenia wlacznie
				// removes all elements, including root
	void usun (T co); // usuwa element z drzewa
					// remove element (value)
	bool sprawdz(); // sprawdza, czy drzewo jest poprawnym drzewem BST
					//checks if tree is correct (rather useful only for debugging)
	T * nast(T czego); // nastepnik zadanego elementu
						// successor of that element
	T * maksimumimum (); // najwiekszy element w drzewie
						//biggest element (and last)
	bool czyJest(T co); // czy cos jest w drzewie
						//check if given element is in tree
	T * minimumimum (); // najmniejszy element w drzewie
						//smallest element (first)
	void dodaj (T co); // dodaje element do drzewa
						// adds (copies)
	void inorder(std::ostream & strum); // wypisuje na zadane wyjscie elementy w porzadku inorder
										//print all elements inorder
	void preorder(std::ostream & strum); // wypisuje na zadane wyjscie elementy w porzadku preorder
										//print all elements preorder
	void postorder(std::ostream & strum); // wypisuje na zadane wyjscie elementy w porzadku postorder
										//print all elements postorder
	void wypiszObficie(std::ostream & strum); //wypisuje dane o kazdym wezle -- wymaga operatora >> dla zawartosci
										//prints info about all nodes - >> operator for T needed
	T * znajdz (T co, bool iter = true); // wyszukuje zadany element
										//search for T
	int size(); //ilosc elementow
				//returns size of tree
	T* operator()(int i) ; //n-ty element przez wskaxnik
							//returns pointer to element with index i
	nodrze<T> & operator()(std::istream & potoczek) ; //zczytanie n elemntow z listy
													//read elements from istream (first must be given amount of elements)
	T& operator[](int i) ; //dostep do obiektu, ale przez wartosc
						//returns value of object with index i
	bool operator+=(T * co); //add
	bool operator+=(T co); //add
	bool operator-=(T co); //remove
	bool operator-=(T * co); //remove
	T* operator%(T * co); // search and return pointer
	bool operator&(T co); // check if exist
	bool operator&(T * co); // check if exist
	template <typename Y, class X> friend Y* operator%(nodrze<Y> & drzewko, X co); // search and return pointer
	void push_back(T co){(*this)+=co;}; // add
};
template <typename T> void nodrze<T>::wypisuj(wezel<T> * w, std::ostream & strum)
{
	if (w==NIL) return;
	wypisuj(w->lewy, strum);

	strum << "Informacje o wezle: "<<flush<<w<<flush;
	if (w->ojciec!=NIL)
		strum <<"\n\tOjciec: "<<(w->ojciec)<<" - "<<*(w->ojciec->zawart);
	else strum <<"\n\tOjciec: NIL";
	if (w->lewy!=NIL)
		strum<<"\n\tLewy syn: "<<(w->lewy)<<" - "<<*(w->lewy->zawart);
	else strum <<"\n\tLewy syn: NIL";
	if (w->prawy!=NIL)
		strum<<"\n\tPrawy syn: "<<(w->prawy)<<" - "<<*(w->prawy->zawart);
	else strum <<"\n\tPrawy syn: NIL";
	strum<<"\n\tZawartosc: "<<*w->zawart;
	strum<<"\n\tKolor: "<<((w->kolor)?(std::string("Czerwony")):(std::string("Czarny")))<<std::endl<<std::endl;

	wypisuj(w->prawy, strum);
}
template <typename T> void nodrze<T>::wypisujPre(wezel<T> * w, std::ostream & strum)
{
	if (w==NIL) return;

	strum << "Informacje o wezle: "<<flush<<w<<flush;
	if (w->ojciec!=NIL)
		strum <<"\n\tOjciec: "<<(w->ojciec)<<" - "<<*(w->ojciec->zawart);
	else strum <<"\n\tOjciec: NIL";
	if (w->lewy!=NIL)
		strum<<"\n\tLewy syn: "<<(w->lewy)<<" - "<<*(w->lewy->zawart);
	else strum <<"\n\tLewy syn: NIL";
	if (w->prawy!=NIL)
		strum<<"\n\tPrawy syn: "<<(w->prawy)<<" - "<<*(w->prawy->zawart);
	else strum <<"\n\tPrawy syn: NIL";
	strum<<"\n\tZawartosc: "<<*w->zawart;
	strum<<"\n\tKolor: "<<((w->kolor)?(std::string("Czerwony")):(std::string("Czarny")))<<std::endl<<std::endl;

	wypisujPre(w->lewy, strum);
	wypisujPre(w->prawy, strum);
}
template <typename T> void nodrze<T>::wypiszObficie(std::ostream & strum)
{
	strum << "Nodrze " <<this<<" ma " << ile << " elementów."<<std::endl;
	strum << "NIL to " << NIL <<std::endl;
	strum << "Ostatnio bralismy "<<ktory<<flush<<" element, czyli "<<" ("<<ostatnio<<")"<<flush<<*ostatnio<<flush<<std::endl;
	strum << "Nasze wezly in-order"<<std::endl;
	wypisujPre(korzen,strum);
};
template <typename T, class X> T* operator%(nodrze<T> & drzewko, X co)
{
	CLOG ("Szukam " <<co <<std::endl);
	drzewko.wypiszObficie(*C->gl->loguj);
	wezel<T> * w = drzewko.korzen;
	while (w!=drzewko.NIL && (*w->zawart)!=co)
	{
		if ((*w->zawart) > co)
			w=w->lewy;
		else w=w->prawy;
	}
	return w->zawart;
};
template <typename T> int nodrze<T>::size()
{
	return ile;
}
template <typename T> void nodrze<T>::clear()
{
	destrukcja(korzen);
	korzen=NIL;
	ostatnio=NIL;
	ktory=0;
}
template <typename T> void nodrze<T>::destrukcja(wezel<T> * w)
{
	if (w==NIL) return;
	destrukcja(w->lewy);
	destrukcja(w->prawy);
	//delete w->zawart;
	delete w;
};
template <typename T> nodrze<T> & nodrze<T>::operator()(std::istream & potoczek)
{
	int temp;
	potoczek >> temp;
	for (int i=0;i<temp;++i)
		potoczek >> (*this);
	return (*this);
}
template <typename T> T* nodrze<T>::operator()(int i)
{
	int j;
	wezel<T> * nasz;
	if (ostatnio!=NIL)
	{
		j=i-ktory;
		if (j>0)
		{
			if (j > (ile-i))
			{
				ktory = i;
				i=ile-i-1;
				nasz = maksimum(korzen);
				for (j=0;j<i;j++)
				{
					nasz = poprzednik(nasz);
				}
				ostatnio=nasz;
				return (nasz->zawart);
			}
			else
			{
				ktory = i;
				nasz = ostatnio;
				for (i=0;i<j;i++)
				{
					nasz = nastepnik(nasz);
				}
				ostatnio=nasz;
				return (nasz->zawart);
			}
		}
		if (j==0)
		{
			return (ostatnio->zawart);
		}
		else
		{
			ktory = i;
			if ((-j)>i)
			{
				nasz = minimum(korzen);
				for (j=0;j<i;j++)
				{
					nasz = nastepnik(nasz);
				}
				ostatnio=nasz;
				return (nasz->zawart);
			}
			else
			{
				nasz = ostatnio;
				for (i=0;i>j;i--)
				{
					nasz = poprzednik(nasz);
				}
				ostatnio=nasz;
				return (nasz->zawart);
			}
		}
	}
	else
	{
		ktory = i;
		nasz = minimum(korzen);
		for (j=0;j<i;j++)
		{
			nasz = nastepnik(nasz);
		}
		ostatnio=nasz;
		return (nasz->zawart);
	}
}
template <typename T> T& nodrze<T>::operator[](int i)
{
	int j;
	wezel<T> * nasz;
	if (ostatnio!=NIL)
	{
		j=i-ktory;
		if (j>0)
		{
			if (j > (ile-i))
			{
				ktory = i;
				i=ile-i-1;
				nasz = maksimum(korzen);
				for (j=0;j<i;j++)
				{
					nasz = poprzednik(nasz);
				}
				ostatnio=nasz;
				return *(nasz->zawart);
			}
			else
			{
				ktory = i;
				nasz = ostatnio;
				for (i=0;i<j;i++)
				{
					nasz = nastepnik(nasz);
				}
				ostatnio=nasz;
				return *(nasz->zawart);
			}
		}
		if (j==0)
		{
			return *(ostatnio->zawart);
		}
		else
		{
			ktory = i;
			if ((-j)>i)
			{
				nasz = minimum(korzen);
				for (j=0;j<i;j++)
				{
					nasz = nastepnik(nasz);
				}
				ostatnio=nasz;
				return *(nasz->zawart);
			}
			else
			{
				nasz = ostatnio;
				for (i=0;i>j;i--)
				{
					nasz = poprzednik(nasz);
				}
				ostatnio=nasz;
				return *(nasz->zawart);
			}
		}
	}
	else
	{
		ktory = i;
		nasz = minimum(korzen);
		for (j=0;j<i;j++)
		{
			nasz = nastepnik(nasz);
		}
		ostatnio=nasz;
		return *(nasz->zawart);
	}
}
template <typename T> bool nodrze<T>::operator+=(T * co)
{
	wezel<T> * w = new wezel<T>(NIL);
	w->kolor=CZERWONY;
	w->zawart = co;
	dodajRBT(w);
	return true;
}
template <typename T> bool nodrze<T>::operator+=(T co)
{
	dodaj(co);
	return true;
}
template <typename T> bool nodrze<T>::operator-=(T co)
{
	usun(co);
	return true;
}
template <typename T> bool nodrze<T>::operator-=(T * co)
{
	usun(*co);
	return true;
}
template <typename T> T* nodrze<T>::operator%(T * co)
{
	wezel<T> * w = szukajIter(korzen,*co);
	if (w != NIL)
		return w;
	else return NULL;
}
template <typename T> bool nodrze<T>::operator&(T co)
{
	return czyJest(co);
}
template <typename T> bool nodrze<T>::operator&(T * co)
{
	return czyJest(*co);
}
template <typename T> class iterator
{
	/*nodrze<T> * dd;
	wezel<T> * akt;
public:
	T * operator->()
	{
		return akt->zawart;
	}
	iterator& operator++()
	{
		akt = dd->nastepnik(akt);
		return this;
	}
	iterator& operator--()
	{
		akt = dd->poprzednik(akt);
		return this;
	}
	T * operator=(T*)
	{
		akt->zawart = T;
		return akt->zawart;
	}*/
	/*void start()
	{
		akt = maksimum(korzen);
	}*/
};
template <typename T> void nodrze<T>::inIt(std::ostream & strum, wezel<T> * wsk)
{
	if (wsk == NIL)
		return;

  // Start from the minimumimum wsk
	while (wsk->lewy != NIL)
		wsk=wsk->lewy;
	do
	{
		visit(wsk);
		// Next in order will be our right child's leftmost child (if NIL, our right child)
		if (wsk->prawy != NIL)
		{
			wsk = wsk->prawy;
			while (wsk->lewy != NIL)
				wsk = wsk->left;
		}
		else
		{
			while (true)
			{
				if (wsk->ojciec == NIL)
				{
					wsk = NIL;
					break;
				}
				wsk = wsk->ojciec;
				// If wsk is its parents left child, then its parent hasn't been visited yet
				if (wsk->ojciec->lewy == wsk)
					break;
			}
		}
	}
	while (wsk != NIL);

};
template <typename T> bool nodrze<T>::sprawdz()
{
	return (sprawdzW(korzen));
};
template <typename T> T * nodrze<T>::znajdz (T co, bool iter)
{
	return ((iter)?(szukajIter(korzen,co)->zawart):(szukajRek(korzen,co)->zawart));
};
template <typename T> void nodrze<T>::usun (T co)
{
	wezel<T> * w = szukajIter(korzen, co);
	usunRBT(w);
	delete w;
}
template <typename T> void nodrze<T>::naprawUsun (wezel<T> * x)
{
	wezel<T> *w;
	while ( (x != korzen)  &&  (x->kolor == CZARNY) )
	{
		CLOG("6... "<<flush);
		if (x == x->ojciec->lewy)
		{
			CLOG("7... "<<flush);
			w = x->ojciec->prawy;
			if (w->kolor == CZERWONY)
			{
				w->kolor = CZARNY;
				x->ojciec->kolor = CZERWONY;
				rotacjaLewa(x->ojciec);
				w = x->ojciec->prawy;
			}
			CLOG("8... "<<flush);
			if ( (w->lewy->kolor == CZARNY)  &&  (w->prawy->kolor == CZARNY) )
			{
				CLOG("8,1... "<<flush);
				w->kolor = CZERWONY;
				x = x->ojciec;
			}
			else
			{
				CLOG("9... "<<flush);
				if (w->prawy->kolor == CZARNY)
				{
					CLOG("9,1... "<<flush);
					w->lewy->kolor = CZARNY;
					w->kolor = CZERWONY;
					rotacjaPrawa(w);
					w = x->ojciec->prawy;
					CLOG("9,2... "<<flush);
				}
				CLOG("9,3... "<<flush);
				w->kolor = x->ojciec->kolor;
				x->ojciec->kolor = CZARNY;
				w->prawy->kolor = CZARNY;
				rotacjaLewa(x->ojciec);
				x=korzen;
				CLOG("9,4... "<<flush);
				
			}
		}
		else
		{
			CLOG("10... "<<flush);
			w = x->ojciec->lewy;
			if (w->kolor == CZERWONY)
			{
				w->kolor = CZARNY;
				x->ojciec->kolor = CZERWONY;
				rotacjaPrawa(x->ojciec);
				w = x->ojciec->lewy;
			}
			CLOG("11... "<<flush);
			if ( (w->lewy->kolor == CZARNY)  &&  (w->prawy->kolor == CZARNY) )
			{
				w->kolor = CZERWONY;
				x = x->ojciec;
			}
			else
			{
				if (w->lewy->kolor == CZARNY)
				{
					w->prawy->kolor = CZARNY;
					w->kolor = CZERWONY;
					rotacjaLewa(w);
					w = x->ojciec->lewy;
				}
				w->kolor = x->ojciec->kolor;
				x->ojciec->kolor = CZARNY;
				w->lewy->kolor = CZARNY;
				rotacjaPrawa(x->ojciec);
				x=korzen;
				CLOG("12... "<<flush);
			}
		}
	}
	x->kolor = CZARNY;
	CLOG("13... "<<flush);
};
template <typename T> wezel<T> * nodrze<T>::usunRBT (wezel<T> * nowy)
{
	CLOG ("Usuwam "<<*nowy->zawart<<std::endl);
	ile--;
	if ((*nowy->zawart) < (*ostatnio->zawart))
	{
		ktory--;
		CLOG("Ostatnio to "<<(*ostatnio->zawart)<<", czyli teraz "<<(ktory)<<" (mniej) element."<<std::endl);
	}
	else if (nowy == ostatnio)
	{
		CLOG ("To by³ ostatnio ogl¹dany element. Elementem o numerze "<<ktory<<" bedzie teraz ");
		if (ktory < ile)
		{
			ostatnio = nastepnik(ostatnio);
		}
		else
		{
			CLOG ("Ojej, koniec. Cofamy siê. "<<std::endl);
			ostatnio = poprzednik(ostatnio);
			ktory--;
		}
		CLOG(*ostatnio->zawart<<std::endl);
	}
	CLOG("1... "<<flush);
	wezel<T> *y, *x;
	if ( (nowy->lewy == NIL)  ||  (nowy->prawy == NIL) )
		y=nowy;
	else y = nastepnik(nowy);
	CLOG("2... "<<flush);
	if (y->lewy != NIL)
		x = y->lewy;
	else x = y->prawy;
	x->ojciec = y->ojciec;
	CLOG("3... "<<flush);
	if (y->ojciec == NIL)
		korzen = x;
	else if (y == y->ojciec->lewy)
		y->ojciec->lewy = x;
	else
		y->ojciec->prawy = x;
	CLOG("4... "<<flush);
	if (y != nowy)
		(*nowy) = (*y); // skopiowanie
	CLOG("5... "<<flush);
	if (y->kolor == CZARNY)
		naprawUsun(x);
	CLOG ("koniec usuwania"<<std::endl);
	return y;
};
template <typename T> void nodrze<T>::naprawWstaw (wezel<T> * nowy)
{
	//CLOG ("Naprawiam po wstawieniu"<<std::endl);
	while (nowy->ojciec->kolor==CZERWONY)
	{
		if (nowy->ojciec == nowy->ojciec->ojciec->lewy) // ojciec nowego lest lewy
		{
			wezel<T> * y = nowy->ojciec->ojciec->prawy;
			if (y->kolor == CZERWONY) // a stryj jest czerwony
			{
				nowy->ojciec->kolor = CZARNY;
				y->kolor = CZARNY;
				nowy->ojciec->ojciec->kolor = CZERWONY;
				nowy = nowy->ojciec->ojciec;
			}
			else
			{
				if (nowy->ojciec->prawy == nowy) // nowy jest prawym synem
				{
					nowy = nowy->ojciec;
					rotacjaLewa(nowy);
				}
				nowy->ojciec->kolor=CZARNY;
				nowy->ojciec->ojciec->kolor=CZERWONY;
				rotacjaPrawa(nowy->ojciec->ojciec);
			}
		}
		else
		{
			wezel<T> * y = nowy->ojciec->ojciec->lewy;
			if (y->kolor == CZERWONY) // a stryj jest czerwony
			{
				nowy->ojciec->kolor = CZARNY;
				y->kolor = CZARNY;
				nowy->ojciec->ojciec->kolor = CZERWONY;
				nowy = nowy->ojciec->ojciec;
			}
			else
			{
				if (nowy->ojciec->lewy == nowy)
				{
					nowy = nowy->ojciec;
					rotacjaPrawa(nowy);
				}
				nowy->ojciec->kolor=CZARNY;
				nowy->ojciec->ojciec->kolor=CZERWONY;
				rotacjaLewa(nowy->ojciec->ojciec);
			}
		}
	}
	korzen->kolor = CZARNY;
}
template <typename T> void nodrze<T>::dodajRBT (wezel<T> * nowy)
{
	//CLOG("Dodaje do drzewa "<<nowy->zawart<<std::endl);
	ile++;
	if ((*nowy->zawart) < (*ostatnio->zawart))
	{
		ktory++;
	}
	wezel<T> * y =NIL, * x = korzen;
	while (x != NIL)
	{
		y=x;
		if ((*nowy->zawart) < (*x->zawart))
			x=x->lewy;
		else x = x->prawy;
	}
	nowy->ojciec = y;
	if (y == NIL)
	{
		korzen=nowy;
		ostatnio=korzen;
		ktory=0;
	}
	else if ((*nowy->zawart) < (*y->zawart))
		y->lewy = nowy;
	else y->prawy = nowy;
	nowy->kolor = CZERWONY;
	naprawWstaw(nowy);
};
template <typename T> void nodrze<T>::dodaj (T co)
{
	wezel<T> * w = new wezel<T>(NIL);
	w->lewy=w->prawy=w->ojciec=NIL;
	w->zawart = new T(co);
	dodajRBT(w);
}
template <typename T> void nodrze<T>::zepsuj()
{
	int pom;
	pom = *korzen->zawart;
	*korzen->zawart = *korzen->prawy->zawart;
	*korzen->prawy->zawart = pom;
}
template <typename T> bool nodrze<T>::czyBST (wezel<T> * w)
{
	if (w->prawy != NIL)
	{
		if ((*w->prawy->zawart) < (*w->zawart))
			return false;
	}
	if (w->lewy != NIL)
	{
		if((*w->lewy->zawart) > (*w->zawart))
			return false;
	}
	return true;
}
template <typename T> bool nodrze<T>::sprawdzW(wezel<T> * w)
{
	bool ret = czyBST(w);
	if (w->prawy != NIL)
		ret&=sprawdzW(w->prawy);
	if (w->lewy != NIL)
		ret&=sprawdzW(w->lewy);
	return ret;
}
template <typename T> void nodrze<T>::rotacjaLewa (wezel<T> * x)
{
	//CLOG("Wykonuje lew¹ rotacjê na "<<x->zawart<<std::endl);
	wezel<T> * y = x->prawy;
	x->prawy = y->lewy; // zamiana lewego poddrzewa y na prawe poddrzewo x
	if (y->lewy != NIL) y->lewy->ojciec = x; // i przypisanie ojcostwa temu poddrzewu
	y->ojciec = x->ojciec; // ojcem y bedzie ojciec x
	if (x->ojciec == NIL)
		korzen = y;
	else if ((x->ojciec->lewy) == x)
		x->ojciec->lewy = y;
	else
		x->ojciec->prawy = y;
	y->lewy = x; // a x bedzie lewym synem y
	x->ojciec = y;
};
template <typename T> void nodrze<T>::rotacjaPrawa (wezel<T> * y)
{
	//CLOG("Wykonuje prawa rotacjê na "<<y->zawart<<std::endl);
	wezel<T> * x = y->lewy;
	y->lewy = x->prawy; // zamiana prawe poddrzewa x na lewe poddrzewo y
	if (x->prawy != NIL) x->prawy->ojciec = y; // i przypisanie ojcostwa temu poddrzewu
	x->ojciec = y->ojciec; // ojcem x bedzie ojciec y
	if (x->ojciec == NIL)
		korzen = x;
	else if ((y->ojciec->lewy) == y)
		y->ojciec->lewy = x;
	else
		y->ojciec->prawy = x;
	x->prawy = y; // a y bedzie prawym synem x
	y->ojciec = x;
};
template <typename T> T * nodrze<T>::nast(T czego)
{
	wezel<T> * w = szukajIter(korzen,czego);
	if (w != NIL)
		w = nastepnik(w);
	else throw std::exception("Nie znaleziono wartosci");
	if (w != NIL)
		return (w->zawart);
	else throw std::exception("Nie znaleziono nastepnika");
};
template <typename T> bool nodrze<T>::czyJest(T co)
{
	if ( szukajIter(korzen,co) != NIL )
		return true;
	else return false;
}
template <typename T> wezel<T> * nodrze<T>::szukajRek(wezel<T> * w, T co)
{
	if (w==NIL || (!(((*w->zawart)<co)||(co<(*w->zawart)))))
		return w;
	if (co < (*w->zawart))
		return szukajRek(w->lewy,co);
	else return szukajRek(w->prawy,co);
};
template <typename T> wezel<T> * nodrze<T>::szukajIter(wezel<T> * w, T co)
{
	while ( w!=NIL && (((*w->zawart)<co)||(co<(*w->zawart))) )
	{
		if (co < (*w->zawart))
			w=w->lewy;
		else w=w->prawy;
	}
	return (w)?w:NULL;
};
template <typename T> wezel<T> * nodrze<T>::minimum(wezel<T> * w)
{
	while (w->lewy != NIL)
		w=w->lewy;
	return w;
};
template <typename T> wezel<T> * nodrze<T>::maksimum(wezel<T> * w)
{
	while (w->prawy != NIL)
		w=w->prawy;
	return w;
};
template <typename T> wezel<T> * nodrze<T>::nastepnik(wezel<T> * w)
{
	if (w->prawy != NIL)
		return minimum(w->prawy);
	wezel<T> * y = w->ojciec;
	while (y!= NIL && w == y->prawy)
	{
		w=y;
		y=y->ojciec;
	}
	return y;
};
template <typename T> wezel<T> * nodrze<T>::poprzednik(wezel<T> * w)
{
	if (w->lewy != NIL)
		return maksimum(w->lewy);
	wezel<T> * y = w->ojciec;
	while (y!= NIL && w == y->lewy)
	{
		w=y;
		y=y->ojciec;
	}
	return y;
};
template <typename T> T * nodrze<T>::maksimumimum ()
{
	wezel<T> * ret = maksimum(korzen);
	if (ret != NIL)
		return (ret->zawart);
	else throw std::exception("Drzewo jest puste");
};
template <typename T> T * nodrze<T>::minimumimum ()
{
	wezel<T> * ret = minimum(korzen);
	if (ret != NIL)
		return (ret->zawart);
	else throw std::exception("Drzewo jest puste");
};
template <typename T> void nodrze<T>::inorder(std::ostream & strum)
{
	in(strum,korzen);
}
template <typename T> void nodrze<T>::preorder(std::ostream & strum)
{
	pre(strum,korzen);
}
template <typename T> void nodrze<T>::postorder(std::ostream & strum)
{
	post(strum,korzen);
}
template <typename T> void nodrze<T>::in(std::ostream & strum, wezel<T> * wsk)
{
	if (wsk==NIL)
		return;
	if (wsk->lewy != NIL)
		in(strum,wsk->lewy);
	strum << *wsk->zawart<<"\t";
	if (wsk->prawy != NIL)
		in(strum,wsk->prawy);
};
template <typename T> void nodrze<T>::post(std::ostream & strum, wezel<T> * wsk)
{
	if (wsk==NIL)
		return;
	if (wsk->lewy != NIL)
		post(strum,wsk->lewy);
	if (wsk->prawy != NIL)
		post(strum,wsk->prawy);
	strum << *wsk->zawart<<"\t";
};
template <typename T> void nodrze<T>::pre(std::ostream & strum, wezel<T> * wsk)
{
	if (wsk == NIL)
		return;
	strum << *wsk->zawart<<"\t";
	if (wsk->lewy != NIL)
		pre(strum,wsk->lewy);
	if (wsk->prawy != NIL)
		pre(strum,wsk->prawy);
};
#endif //_NODRZE_H