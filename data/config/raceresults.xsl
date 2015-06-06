<?xml version="1.0" encoding="utf-8"?>

<!--
    file                 : credits.xml
    created              : Thu Mar 19 22:00:00 CEST 2009
    copyright            : (C) 2009 by Eugen Treise
    web                  : speed-dreams.sourceforge.net
    version              : $Id: raceresults.xsl 5121 2013-02-10 16:16:25Z pouillot $
-->

<!-- This xsl file was made to format results-*.xml files, -->
<!-- which are created by Speed Dreams after a race, into readable html files. -->
<!-- Usage: Open results-*.xml file (it must have a reference to this file) -->
<!--        in your browser, or use an xslt processor to convert the xml file -->
<!--        to an html file. -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="html"/>

<!-- Template for the params element -->
<xsl:template match="/params">

	<html>
	<head><title>Speed Dreams Race Results</title></head>
	<body>

	<p><A HREF="http://www.speed-dreams.org">Speed Dreams Home Page</A></p>

	<!-- print Params attributes -->
	<xsl:for-each select="@*">
		<xsl:value-of select="name()"/> = <xsl:value-of select="."/><br/>
	</xsl:for-each>

	<!-- select the template by section name and call it -->
	<xsl:for-each select="section">
		<xsl:variable name="Section" select="@name"/>
		<HR/>
		<xsl:choose>
			<xsl:when test="$Section = 'Header' or $Section = 'Current'">
				<xsl:call-template name="Header"/>
			</xsl:when>
			<xsl:when test="$Section = 'Standings'">
				<xsl:call-template name="Standings"/>
				<!-- Results for multiple tracks available -->
				<xsl:if test="../section[@name='Current']/attnum[@name='current track']/@val > 1">
					<HR/>
					<xsl:call-template name="Summary"/>
				</xsl:if>
			</xsl:when>
			<xsl:otherwise>
				<xsl:call-template name="Track"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:for-each>

	</body>
	</html>
</xsl:template>


<!-- Template for Header and Current sections -->
<xsl:template name="Header">
	<h4>Section <xsl:value-of select="@name"/>:</h4>
	<!-- print all attributes and their values of all child elements in the current section -->
	<xsl:for-each select="*">
		<xsl:value-of select="@name"/> = <xsl:value-of select="@val"/><br/>
	</xsl:for-each>
</xsl:template>


<!-- Template for "Trackname" sections -->
<!-- Call different templates for Practice and other Race sessions. -->
<xsl:template name="Track">
	<h2>Track: <xsl:value-of select="@name"/></h2>
	<xsl:choose>
		<xsl:when test="section[@name='Results']/section[1]/@name = 'Practice'">
			<xsl:call-template name="Practice"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:call-template name="Race"/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>


<!-- Template for Qualifying and Race sections with several drivers -->
<xsl:template name="Race">
	<xsl:for-each select="section[@name='Results']/section">
		<h3><xsl:value-of select="@name"/></h3>
		<xsl:for-each select="*[@val]">
			<xsl:value-of select="@name"/> = <xsl:value-of select="@val"/><br/>
		</xsl:for-each>
		<p>
		<table border="1">
		<!-- Table Head -->
		<tr>
		<th>Rank</th>
		<xsl:for-each select="section[@name='Rank']/section[1]/*">
			<th><xsl:value-of select="@name"/></th>
		</xsl:for-each>
		</tr>
		<xsl:for-each select="section[@name='Rank']/section">
			<!-- choose an other format for human driver, for drivers with points and for drivers with high damage -->
			<tr>
			<xsl:attribute name="style">
				<xsl:if test="attstr[@name='module']/@val = 'human'">
					<xsl:text>font-weight:bold;</xsl:text>
				</xsl:if>
				<xsl:if test="attnum[@name='points']/@val > 0">
					<xsl:text>color:blue;</xsl:text>
				</xsl:if>
				<xsl:if test="attnum[@name='dammages']/@val > 10000">
					<xsl:text>background-color:lightgrey</xsl:text>
				</xsl:if>
			</xsl:attribute>

			<!-- Table content (row) -->
			<xsl:choose>
				<xsl:when test="contains(../../@name, 'Race')">
					<xsl:variable name="PosGain" select="attnum[@name='index']/@val + 1 - @name"/>
					<xsl:variable name="PosSymbol">
						<xsl:choose>
							<xsl:when test="$PosGain > 0">
								<xsl:value-of select="'▲'"/>
							</xsl:when>
							<xsl:when test="$PosGain &lt; 0">
								<xsl:value-of select="'▼'"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="'-'"/>
							</xsl:otherwise>
						</xsl:choose>
					</xsl:variable>
					<td>
					<xsl:if test="$PosGain != 0">
						<xsl:attribute name="style">
							<xsl:text>color:</xsl:text>
							<xsl:choose>
								<xsl:when test="$PosGain > 0">
									<xsl:value-of select="'green'"/>
								</xsl:when>
								<xsl:otherwise>
									<xsl:value-of select="'red'"/>
								</xsl:otherwise>
							</xsl:choose>
						</xsl:attribute>
					</xsl:if>
					<xsl:value-of select="@name"/>
					<xsl:text> (</xsl:text>
					<xsl:value-of select="$PosSymbol"/>
					<xsl:choose>
						<xsl:when test="$PosGain > 0">
							<xsl:value-of select="$PosGain"/>
						</xsl:when>
						<xsl:when test="$PosGain &lt; 0">
							<xsl:value-of select="-$PosGain"/>
						</xsl:when>
					</xsl:choose>
					<xsl:text>)</xsl:text>
					</td>
				</xsl:when>
				<xsl:otherwise>
					<td><xsl:value-of select="@name"/></td>
				</xsl:otherwise>
			</xsl:choose>
			<xsl:for-each select="*">
				<td>
				<xsl:call-template name="FormatedOutput">
					<xsl:with-param name="TimeDiff" select="1"/>
				</xsl:call-template>
				</td>
			</xsl:for-each>
			</tr>
		</xsl:for-each>
		</table>
		</p>
	</xsl:for-each>
</xsl:template>


<!-- Template for Practice section -->
<xsl:template name="Practice">
	<xsl:for-each select="section[@name='Results']/section">
		<h3><xsl:value-of select="@name"/></h3>
		Driver: <xsl:value-of select="attstr[@name='driver name']/@val"/><br/>
		<xsl:variable name="TotalLaps" select="count(section)"/>
		<xsl:variable name="TotalTime" select="sum(section/attnum[@name='time']/@val)"/>
		<xsl:value-of select="$TotalLaps"/> laps in <xsl:value-of select="floor($TotalTime div 60)"/> min <xsl:value-of select="round($TotalTime mod 60)"/> s<br/>
		<p>
		<table border="1">
		<!-- Table Head -->
		<tr>
		<th>Lap</th>
		<th>time diff</th>
		<xsl:for-each select="section[1]/*">
			<th><xsl:value-of select="@name"/></th>
		</xsl:for-each>
		</tr>

		<!-- Table content (row) -->
		<xsl:for-each select="section">
			<tr>
			<!-- fastest lap -->
			<xsl:if test="attnum[@name='time']/@val = ../section[$TotalLaps]/attnum[@name='best lap time']/@val">
				<xsl:attribute name="style">
					<xsl:text>font-weight:bold</xsl:text>
				</xsl:attribute>
			</xsl:if>
			<td><xsl:value-of select="@name"/></td>
			<td>
			<xsl:call-template name="FormatTime">
				<xsl:with-param name="time" select="attnum[@name='time']/@val - ../section[$TotalLaps]/attnum[@name='best lap time']/@val"/>
			</xsl:call-template>
			</td>
			<xsl:for-each select="*">
				<td><xsl:call-template name="FormatedOutput"/></td>
			</xsl:for-each>
			</tr>
		</xsl:for-each>
		</table>
		</p>
	</xsl:for-each>
</xsl:template>


<!-- Template for Standings section -->
<xsl:template name="Standings">
	<xsl:variable name="TrackCount" select="../section[@name='Current']/attnum[@name='current track']/@val"/>
	<h3><xsl:value-of select="@name"/>
	<xsl:if test="$TrackCount > 1">
		after <xsl:value-of select="$TrackCount"/> races
	</xsl:if>
	</h3>
	<p>
	<table border="1">
	<!-- Table header -->
	<tr>
		<th>Rank</th>
		<th>Name</th>
		<th>Points</th>
	</tr>

	<xsl:for-each select="*">
		<!-- highlight player and drivers with points -->
		<tr>
		<xsl:attribute name="style">
			<xsl:if test="attstr[@name='module']/@val = 'human' or attnum[@name='points']/@val > 0">
				<xsl:text>font-weight:bold;</xsl:text>
			</xsl:if>
			<xsl:text>color:</xsl:text>
			<xsl:choose>
				<xsl:when test="attstr[@name='module']/@val = 'human'">
					<xsl:value-of select="'red'"/>
				</xsl:when>
				<xsl:when test="attnum[@name='points']/@val > 0">
					<xsl:value-of select="'blue'"/>
				</xsl:when>
				<xsl:otherwise>
					<xsl:value-of select="'black'"/>
				</xsl:otherwise>
			</xsl:choose>
		</xsl:attribute>

		<!-- Table content (row) -->
		<td><xsl:value-of select="@name"/></td>
		<td><xsl:value-of select="attstr[@name='name']/@val"/></td>
		<td><xsl:value-of select="attnum[@name='points']/@val"/></td>
		</tr>
	</xsl:for-each>

	</table>
	</p>
</xsl:template>

<!-- Show drivers positions over multiple race events -->
<xsl:template name="Summary">
	<h3>Championship Summary</h3>
	<p>
	<table border="1">
	<!-- Table header -->
	<tr>
	<th>Driver/Track ->Rank</th>
	<!-- List all track names -->
	<xsl:for-each select="../section[@name!='Header' and @name!='Current' and @name!='Standings']">
		<th><xsl:value-of select="@name"/></th>
	</xsl:for-each>
	</tr>
	<xsl:for-each select="*/attstr[@name='name']">
		<xsl:variable name="DriverName" select="@val"/>
		<tr>
		<!-- choose an other format for human driver -->
		<xsl:if test="../attstr[@name='module']/@val = 'human'">
			<xsl:attribute name="style">
				<xsl:text>font-weight:bold</xsl:text>
			</xsl:attribute>
		</xsl:if>

		<td><xsl:value-of select="$DriverName"/></td>
		<xsl:for-each select="//section[@name='Results']/section[@name='Race']/section[@name='Rank']/section/attstr[@name='name' and @val=$DriverName]">
			<td>
			<xsl:if test="../attnum[@name='points']/@val > 0">
				<xsl:attribute name="style">
					<xsl:text>color:</xsl:text>
					<xsl:choose>
						<!-- first place -->
						<xsl:when test="../@name = 1">
							<xsl:value-of select="'red'"/>
						</xsl:when>
						<xsl:otherwise>
							<xsl:value-of select="'blue'"/>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:attribute>
			</xsl:if>
			<xsl:value-of select="../@name"/>
			</td>
		</xsl:for-each>
		</tr>
	</xsl:for-each>
	</table>
	</p>
</xsl:template>


<!-- Print values; speed and time will be formated. -->
<xsl:template name="FormatedOutput">
	<!-- When parameter TimeDiff is != 0, show the values of the 'time' column as difference to the value in the first row. -->
	<xsl:param name="TimeDiff" select="0"/>
	<xsl:choose>
		<xsl:when test="contains(@name,'speed')">
			<!-- convert speed from m/s to km/h -->
			<xsl:value-of select="round(3.6*@val)"/>
		</xsl:when>
		<xsl:when test="@name = 'time' and $TimeDiff and ../@name != 1">
			<xsl:variable name="TotalLaps" select="../../section[1]/attnum[@name='laps']/@val"/>
			<xsl:choose>
				<xsl:when test="$TotalLaps > ../attnum[@name='laps']/@val">
					<!-- when the winner overlapped this driver, show the lap count difference -->
					<xsl:text>+</xsl:text>
					<xsl:variable name="LapsBehind" select="$TotalLaps - ../attnum[@name='laps']/@val"/>
					<xsl:value-of select="$LapsBehind"/>
					<xsl:text> Lap</xsl:text>
					<xsl:if test="$LapsBehind > 1">s</xsl:if>
				</xsl:when>
				<xsl:otherwise>
					<!-- show the time difference to the winner with a plus sign -->
					<xsl:text>+</xsl:text>
					<xsl:call-template name="FormatTime">
						<xsl:with-param name="time" select="@val - ../../section[1]/attnum[@name='time']/@val"/>
					</xsl:call-template>
				</xsl:otherwise>
			</xsl:choose>
		</xsl:when>
		<xsl:when test="contains(@name,'time')">
			<!-- print time -->
			<xsl:call-template name="FormatTime">
				<xsl:with-param name="time" select="@val"/>
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="@val"/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>


<!-- Converts positive time values from s to min:s or h min. -->
<xsl:template name="FormatTime">
	<xsl:param name="time"/>
	<xsl:variable name="Minutes" select="floor($time div 60)"/>
	<xsl:choose>
		<xsl:when test="$Minutes &lt; 60">
			<xsl:value-of select="$Minutes"/>
			<xsl:text>:</xsl:text>
			<!-- (mod returns not exact values, therefore format to have 3 digits behind the decimal point) -->
			<xsl:value-of select="format-number(($time mod 60),'00.000')"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="floor($Minutes div 60)"/>h
			<xsl:value-of select="$Minutes mod 60"/>min
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

</xsl:stylesheet>
