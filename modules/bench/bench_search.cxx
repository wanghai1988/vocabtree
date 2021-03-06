#include <config.hpp>
#include "bench_config.hpp"

#include <fstream>
#include <utils/filesystem.hpp>
#include <utils/numerics.hpp>
#include <utils/dataset.hpp>
#include <utils/misc.hpp>
#include <utils/vision.hpp>
#include <utils/logger.hpp>
#include <utils/image.hpp>
#include <utils/cycletimer.hpp>
#include <search/bag_of_words/bag_of_words.hpp>
#include <search/inverted_index/inverted_index.hpp>
#include <vis/matches_page.hpp>

_INITIALIZE_EASYLOGGINGPP


void bench_oxford() {

#if ENABLE_MULTITHREADING && ENABLE_MPI
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

	MatchesPage html_output;
	
  const uint32_t num_clusters = s_oxfordmini_num_clusters;

	SimpleDataset oxford_dataset(s_oxfordmini_data_dir, s_oxfordmini_database_location, 256);
	LINFO << oxford_dataset;

	std::stringstream index_output_file;
	index_output_file << oxford_dataset.location() << "/index/" << num_clusters << ".index";
	InvertedIndex ii(index_output_file.str());

	double total_time = 0.0;
	uint32_t num_validate = 16;
	uint32_t total_iterations = 256;
	// const std::vector<PTR_LIB::shared_ptr<const Image> > &rand_images = oxford_dataset.random_images(256);
	for(uint32_t i=0; i<total_iterations; i++) {
		std::cout << PerfTracker::instance() << std::endl;
		PTR_LIB::shared_ptr<InvertedIndex::MatchResults> matches = 
			std::static_pointer_cast<InvertedIndex::MatchResults>(ii.search(oxford_dataset, PTR_LIB::shared_ptr<const SearchParamsBase>(), oxford_dataset.image(i)));

		if(!matches) {
			LERROR << "Error while running search.";
			continue;
   		 }
#if ENABLE_MULTITHREADING && ENABLE_MPI
    if (rank != 0)
      continue;
#endif
		// validate matches
		cv::Mat keypoints_0, descriptors_0;
		PTR_LIB::shared_ptr<SimpleDataset::SimpleImage> query_image = std::static_pointer_cast<SimpleDataset::SimpleImage>(oxford_dataset.image(i));
		const std::string &query_keypoints_location = oxford_dataset.location(query_image->feature_path("keypoints"));
		const std::string &query_descriptors_location = oxford_dataset.location(query_image->feature_path("descriptors"));
		filesystem::load_cvmat(query_keypoints_location, keypoints_0);
		filesystem::load_cvmat(query_descriptors_location, descriptors_0);

    std::vector<int> validated(num_validate, 0);  
#if ENABLE_MULTITHREADING && ENABLE_OPENMP
		#pragma omp parallel for schedule(dynamic)
#endif
		for(int32_t j=0; j<(int32_t)num_validate; j++) {
			cv::Mat keypoints_1, descriptors_1;
			PTR_LIB::shared_ptr<SimpleDataset::SimpleImage> match_image = std::static_pointer_cast<SimpleDataset::SimpleImage>(oxford_dataset.image(matches->matches[j]));
			const std::string &match_keypoints_location = oxford_dataset.location(match_image->feature_path("keypoints"));
			const std::string &match_descriptors_location = oxford_dataset.location(match_image->feature_path("descriptors"));
			filesystem::load_cvmat(match_keypoints_location, keypoints_1);
			filesystem::load_cvmat(match_descriptors_location, descriptors_1);

			cv::detail::MatchesInfo match_info;
			vision::geo_verify_f(descriptors_0, keypoints_0, descriptors_1, keypoints_1, match_info);

			validated[j] = vision::is_good_match(match_info) ? 1 : -1;
		}

		html_output.add_match(i, matches->matches, oxford_dataset, PTR_LIB::make_shared< std::vector<int> >(validated));
		html_output.write(oxford_dataset.location() + "/results/matches/");
	}

	


	// Write out the timings
	std::stringstream timings_file_name;
	timings_file_name << oxford_dataset.location() + "/results/times.index." <<  ii.num_clusters() << ".csv";
	std::ofstream ofs(timings_file_name.str(), std::ios::app);
	if((size_t)ofs.tellp() == 0) {
		std::stringstream header;
		header << "machine\ttime(s)\titerations\tmultithreading\topenmp\tmpi\t" << std::endl;
		ofs.write(header.str().c_str(), header.str().size());
	}
	std::stringstream timing;
	timing << misc::get_machine_name() << "\t" << total_time << "\t" << total_iterations << "\t" << 
			  ENABLE_MULTITHREADING << "\t" << ENABLE_OPENMP << 
			  "\t" << ENABLE_MPI << "\t" << std::endl;
	ofs.write(timing.str().c_str(), timing.str().size());
	ofs.close();

	// validate matches

}

int main(int argc, char *argv[]) {
#if ENABLE_MULTITHREADING && ENABLE_MPI
  MPI_Init(&argc, &argv);
#endif

	bench_oxford();

#if ENABLE_MULTITHREADING && ENABLE_MPI
	MPI_Finalize();
#endif
	return 0;
}