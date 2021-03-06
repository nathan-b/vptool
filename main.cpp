#include <filesystem>
#include <iostream>
#include <list>
#include <random>
#include <sstream>

#include "operation.h"
#include "scoped_tempdir.h"
#include "vp_parser.h"

bool dump_index(const vp_index* idx)
{
	std::cout << idx->print_index_listing() << std::endl;
	return true;
}

bool dump_file(const vp_index* idx, const std::string& filename, const std::string outfilename)
{
	vp_file* f = idx->find(filename);

	if (!f) {
		std::cerr << "Could not find " << filename << " in " << idx->to_string() << std::endl;
		return false;
	}

	if (outfilename.empty()) {
		// Dump to console
		std::cout << f->dump() << std::endl;
	} else {
		// Dump to file (i.e. extract)
		return f->dump(outfilename);
	}

	return true;
}

bool dump_file(const vp_index* idx, const std::string& filename)
{
	return dump_file(idx, filename, "");
}

bool extract_all(const vp_index* idx, const std::string& outpath)
{
	return idx->dump(outpath);
}

bool build_package(const std::string& vp_filename, const std::string& src_path)
{
	// Try to find the data directory
	std::filesystem::path p(src_path);

	if (std::filesystem::exists(p / "data")) {
		p.append("data");
	} else if (*(--p.end()) != "data") {
		std::cerr << "Warning: could not find data directory. Assuming target of " << p << std::endl;
	}

	if (!std::filesystem::exists(p) || !std::filesystem::is_directory(p)) {
		std::cerr << p << " does not exist or is not a directory" << std::endl;
		return false;
	}

	//std::cout << "Building package from " << p << std::endl;

	vp_index idx;
	return idx.build(p, vp_filename);
}

bool replace_file(vp_index* idx, const std::string& filename, const std::string& infilename)
{
	// There's a sneaky optimization we can use here: if the updated file is
	// the same size or smaller than the original, we can just overwrite the file
	// data inside the package and update the size in the index. That potentially
	// results in a bit of wastage in the file data segment, but no big deal.
	vp_file* currfile = idx->find(filename);
	if (!currfile) {
		std::cerr << "Could not find " << filename << " in package!\n";
		return false;
	}

	std::filesystem::directory_entry direntry(infilename);

	if (currfile->get_size() >= direntry.file_size()) {
		// Update the file
		if (!currfile->write_file_contents(direntry.path())) {
			std::cerr << "Could not write file contents to package for " << filename << std::endl;
			return false;
		}
		if (!idx->update_index(currfile)) {
			std::cerr << "Could not update index entry for " << filename << std::endl;
			return false;
		}
		return true;
	}

	// vp files don't tend to be massive (I'll probably regret those words at some point)
	// so for maximum reliability, just extract the whole thing, replace the file, and
	// then build the new file over top of the old.
	auto tmpd = scoped_tempdir("vptool-");
	if (!std::filesystem::is_directory(tmpd)) {
		std::cerr << "Could not create a temporary directory\n";
		return false;
	}

	if (!idx->dump(tmpd)) {
		std::cerr << "Could not dump package file to " << tmpd << std::endl;
		return false;
	}

	// Replace the file with the new file
	std::filesystem::path f = tmpd / currfile->get_path();
	if (!std::filesystem::exists(f)) {
		std::cerr << "Could not find " << filename << " at path " << f << std::endl;
		return false;
	}
	if (!std::filesystem::copy_file(infilename, f, std::filesystem::copy_options::overwrite_existing)) {
		std::cerr << "Could not copy " << infilename << " over " << f << std::endl;
		return false;
	}

	// Repackage the whole dealio
	return build_package(idx->get_filename(), tmpd);
}

static void usage()
{
	std::cout << "Usage: vptool <operation> <vp_file> [options]\n"
			  << "  Valid operations: t / dump-index             Print the index of the package file\n"
			  << "                    d / dump-file  <-f filename>  Dump the contents of a single file in the package\n"
			  << "                    f / extract-file <-f filename> <-o output-file>  Extract the contents of a single file to disk\n"
			  << "                    x / extract-all  [-o output-path]  Extract the entire package to the output path (or current directory)\n"
			  << "                    r / replace-file <-f filename> <-i input-file>  Replace the contents of a single file\n"
			  << "                    p / build-package <-i input-path>  Build a new vp file with the contents of input-path\n";
}

int main(int argc, char** argv)
{
	operation op;

	if (!op.parse(argc, argv)) {
		std::cerr << "Command line input error\n";
		usage();
		return -1;
	}

	if (op.get_type() == BUILD_PACKAGE) {
		// Since the arguments can be a little confusing, if the user did not specify
		// a vp file but did specify an output file, we know what to do
		std::string vpfile = op.get_package_filename();
		if (vpfile.empty() && !op.get_dest_path().empty()) {
			vpfile = op.get_dest_path();
		}

		if (vpfile.empty()) {
			std::cerr << "Please specify a filename for the new package\n";
			usage();
			return -1;
		}
		// Build package operations don't parse an index file beforehand
		if (!build_package(op.get_package_filename(), op.get_src_filename())) {
			std::cerr << "Error building package " << op.get_package_filename() << std::endl;
			return -2;
		}
		std::cout << "Success!\n";
		return 0;
	}

	// Parse the index file
	vp_index* idx = new vp_index();
	if (!idx->parse(op.get_package_filename())) {
		std::cerr << "Error parsing " << op.get_package_filename() << std::endl;
		delete idx;
		return -2;
	}

	bool ret = false;
	switch (op.get_type()) {
	case DUMP_INDEX:
		ret = dump_index(idx);
		break;
	case DUMP_FILE:
		ret = dump_file(idx, op.get_internal_filename());
		break;
	case EXTRACT_FILE:
		ret = dump_file(idx, op.get_internal_filename(), op.get_dest_path());
		break;
	case EXTRACT_ALL:
		ret = extract_all(idx, op.get_dest_path());
		break;
	case REPLACE_FILE:
		ret = replace_file(idx, op.get_internal_filename(), op.get_src_filename());
		break;
	default:
		return -1;
	}

	if (!ret) {
		std::cerr << "Operation did not complete successfully!\n";
	}

	// Cleanup
	delete idx;

	return 0;
}
