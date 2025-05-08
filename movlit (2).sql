-- phpMyAdmin SQL Dump
-- version 5.2.1
-- https://www.phpmyadmin.net/
--
-- Host: 127.0.0.1
-- Generation Time: May 08, 2025 at 06:39 PM
-- Server version: 10.4.32-MariaDB
-- PHP Version: 8.2.12

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `movlit`
--

-- --------------------------------------------------------

--
-- Table structure for table `account_tbl`
--

CREATE TABLE `account_tbl` (
  `account_id` int(11) NOT NULL,
  `username` varchar(50) NOT NULL,
  `password` text NOT NULL,
  `email` varchar(250) NOT NULL,
  `access` enum('admin','staff','device') NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `account_tbl`
--

INSERT INTO `account_tbl` (`account_id`, `username`, `password`, `email`, `access`) VALUES
(1, 'mark', 'mark', 'mark@gmail.com', 'admin'),
(2, 'device', 'device', 'okirapew@gmail.com', 'device');

-- --------------------------------------------------------

--
-- Table structure for table `lighting_logs`
--

CREATE TABLE `lighting_logs` (
  `log_id` int(11) NOT NULL,
  `room_id` int(11) NOT NULL,
  `account_id` int(11) NOT NULL,
  `date` timestamp NOT NULL DEFAULT current_timestamp(),
  `set_to_state` tinyint(4) NOT NULL,
  `log_date` timestamp NOT NULL DEFAULT current_timestamp()
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- --------------------------------------------------------

--
-- Table structure for table `profile_tbl`
--

CREATE TABLE `profile_tbl` (
  `account_id` int(11) NOT NULL,
  `first_name` varchar(100) NOT NULL,
  `middle_name` varchar(100) NOT NULL,
  `last_name` varchar(100) NOT NULL,
  `suffix` varchar(100) NOT NULL,
  `image_path` varchar(255) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- --------------------------------------------------------

--
-- Table structure for table `room_lighting`
--

CREATE TABLE `room_lighting` (
  `room_id` int(11) NOT NULL,
  `light_state` tinyint(4) NOT NULL DEFAULT 0,
  `is_used` tinyint(4) NOT NULL DEFAULT 0,
  `set_to_state` tinyint(1) NOT NULL DEFAULT 0,
  `date_updated` timestamp NOT NULL DEFAULT current_timestamp()
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- --------------------------------------------------------

--
-- Table structure for table `room_tbl`
--

CREATE TABLE `room_tbl` (
  `room_id` int(11) NOT NULL,
  `room_number` varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `room_tbl`
--

INSERT INTO `room_tbl` (`room_id`, `room_number`) VALUES
(1, 'ROOM 203');

-- --------------------------------------------------------

--
-- Table structure for table `schedule_tbl`
--

CREATE TABLE `schedule_tbl` (
  `schedule_id` int(11) NOT NULL,
  `room_id` int(11) NOT NULL,
  `section_id` int(11) NOT NULL,
  `account_id` int(11) NOT NULL,
  `mon` tinyint(1) NOT NULL DEFAULT 0,
  `tue` tinyint(1) NOT NULL DEFAULT 0,
  `wed` tinyint(1) NOT NULL DEFAULT 0,
  `thu` tinyint(1) NOT NULL DEFAULT 0,
  `fri` tinyint(1) NOT NULL DEFAULT 0,
  `sat` tinyint(1) NOT NULL DEFAULT 0,
  `sun` tinyint(1) NOT NULL DEFAULT 0,
  `one_time` tinyint(1) NOT NULL DEFAULT 0,
  `start` time NOT NULL,
  `end` time NOT NULL,
  `specific_date` date DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- --------------------------------------------------------

--
-- Table structure for table `section_tbl`
--

CREATE TABLE `section_tbl` (
  `section_id` int(11) NOT NULL,
  `section_name` varchar(255) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `account_tbl`
--
ALTER TABLE `account_tbl`
  ADD PRIMARY KEY (`account_id`),
  ADD UNIQUE KEY `username` (`username`);

--
-- Indexes for table `lighting_logs`
--
ALTER TABLE `lighting_logs`
  ADD PRIMARY KEY (`log_id`);

--
-- Indexes for table `profile_tbl`
--
ALTER TABLE `profile_tbl`
  ADD KEY `account_id` (`account_id`);

--
-- Indexes for table `room_lighting`
--
ALTER TABLE `room_lighting`
  ADD UNIQUE KEY `room_id` (`room_id`);

--
-- Indexes for table `room_tbl`
--
ALTER TABLE `room_tbl`
  ADD PRIMARY KEY (`room_id`);

--
-- Indexes for table `section_tbl`
--
ALTER TABLE `section_tbl`
  ADD PRIMARY KEY (`section_id`),
  ADD UNIQUE KEY `section_name` (`section_name`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `account_tbl`
--
ALTER TABLE `account_tbl`
  MODIFY `account_id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=3;

--
-- AUTO_INCREMENT for table `lighting_logs`
--
ALTER TABLE `lighting_logs`
  MODIFY `log_id` int(11) NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT for table `room_tbl`
--
ALTER TABLE `room_tbl`
  MODIFY `room_id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=2;

--
-- AUTO_INCREMENT for table `section_tbl`
--
ALTER TABLE `section_tbl`
  MODIFY `section_id` int(11) NOT NULL AUTO_INCREMENT;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
