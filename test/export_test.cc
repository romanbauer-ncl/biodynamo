#include <new>

#include "gtest/gtest.h"
#include "cell.h"
#include "exporter.h"

#include <fstream>
#include <iostream>

namespace bdm {

	TEST(ExportTest, ConductExportToFile) {
		using real_v = VcBackend::real_v;
		using real_t = real_v::value_type;
		if (real_v::Size < 2) {
			FAIL() << "Backend must at least support two elements for this test";
		}
  // set up cells
		real_v diameter((const real_t[]){10, 10});
		std::array<real_v, 3> position = {real_v((const real_t[]){0.5, -5}),
		real_v((const real_t[]){1, 5}),
		real_v((const real_t[]){0, 0.9})};

		Cell<VcBackend> cell(diameter);
		cell.SetPosition(position);
		daosoa<Cell, VcBackend> cells;
		cells.push_back(cell);

		Exporter exporter;
		exporter.conductExportToFile(&cells, "TestExporter.dat");

		std::ifstream t;
		std::stringstream buffer;
		t.open("../output/TestExporter.dat");
		std::string line;
		std::getline(t,line);
		EXPECT_EQ(line,"[0.5,1,0]");
		std::getline(t,line);
		EXPECT_EQ(line,"[-5,5,0.9]");
		t.close();
		remove("../output/TestExporter.dat");
	}

}
